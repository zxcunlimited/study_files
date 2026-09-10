#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <string>


struct Client {
    SOCKET socket;
    char peer[64];
    std::string recv_buf;
    std::string send_buf;
    bool got_put;
    bool stop_client;
};

static std::vector<Client *> clients;
static FILE* g_outfile = NULL;

static void set_nonblocking(SOCKET s)
{
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}

static void write_msg_line(const char* peer, uint32_t date, uint32_t time1, uint32_t time2, const std::string& msg) {
    if (!g_outfile) return;

    int dd = date % 100;
    int mm = (date / 100) % 100;
    int yyyy = date / 10000;

    int s1 = time1 % 100;
    int m1 = (time1 / 100) % 100;
    int h1 = time1 / 10000;

    int s2 = time2 % 100;
    int m2 = (time2 / 100) % 100;
    int h2 = time2 / 10000;

    fprintf(g_outfile, "%s %02d.%02d.%04d %02d:%02d:%02d %02d:%02d:%02d %s\n",
            peer, dd, mm, yyyy, h1, m1, s1, h2, m2, s2, msg.c_str());
    fflush(g_outfile);
}

static int parse_message(Client* client) {
    if (client->recv_buf.size() < 20) return 0;

    const unsigned char* data = (const unsigned char*)client->recv_buf.data();
    
    uint32_t index, date, time1, time2, msg_len;
    memcpy(&index, data, 4);
    memcpy(&date, data + 4, 4);
    memcpy(&time1, data + 8, 4);
    memcpy(&time2, data + 12, 4);
    memcpy(&msg_len, data + 16, 4);

    index = ntohl(index);
    date = ntohl(date);
    time1 = ntohl(time1);
    time2 = ntohl(time2);
    msg_len = ntohl(msg_len);
    
    if (client->recv_buf.size() < 20 + msg_len) return 0;

    std::string msg(client->recv_buf.data() + 20, msg_len);
    client->recv_buf.erase(0, 20 + msg_len);

    (void)index;
    write_msg_line(client->peer, date, time1, time2, msg);
    client->send_buf += "ok";

    if (msg.size() == 4 && msg == "stop") {
        client->stop_client = true;
    }

    return 1;
}

static int process_input(Client* client) {
    if (!client->got_put) {
        if (client->recv_buf.size() < 3) return 0;
        if (client->recv_buf.compare(0, 3, "put") != 0) return -1;
        client->recv_buf.erase(0, 3);
        client->got_put = true;
    }

    while (true) {
        int res = parse_message(client);
        if (res <= 0) return res;
    }
}

static bool try_flush_send(Client* client)
{
    while (!client->send_buf.empty()) {
        int sent = send(client->socket, client->send_buf.data(), (int)client->send_buf.size(), 0);
        if (sent > 0) {
            client->send_buf.erase(0, (size_t)sent);
            continue;
        }
        if (WSAGetLastError() == WSAEWOULDBLOCK) return true;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s PORT\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    g_outfile = fopen("msg.txt", "a");
    if (!g_outfile) {
        printf("Warning: cannot open msg.txt for writing.\n");
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) {
        printf("Error creating socket.\n");
        return 1;
    }

    int reuse = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_socket, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(listen_socket, SOMAXCONN) != 0) {
        printf("Error bind/listen on port %d\n", port);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    set_nonblocking(listen_socket);
    printf("Listening on port %d\n", port);

    bool global_stop = false;
    bool listen_closed = false;

    while (true) {
        const bool poll_listen = !listen_closed;

        std::vector<WSAPOLLFD> pollfds;
        pollfds.reserve((poll_listen ? 1u : 0u) + clients.size());

        if (poll_listen) {
            WSAPOLLFD ls = {};
            ls.fd = listen_socket;
            ls.events = POLLRDNORM;
            pollfds.push_back(ls);
        }
        for (size_t i = 0; i < clients.size(); ++i) {
            WSAPOLLFD e = {};
            e.fd = clients[i]->socket;
            e.events = POLLRDNORM;
            if (!clients[i]->send_buf.empty()) {
                e.events |= POLLWRNORM;
            }
            pollfds.push_back(e);
        }

        if (pollfds.empty()) break;
        if (WSAPoll(pollfds.data(), (ULONG)pollfds.size(), 100) < 0) break;

        size_t base = 0;
        if (poll_listen) {
            if (pollfds[0].revents & POLLRDNORM) {
                while (true) {
                    sockaddr_in caddr;
                    int caddr_len = sizeof(caddr);
                    SOCKET cs = accept(listen_socket, (sockaddr*)&caddr, &caddr_len);
                    if (cs == INVALID_SOCKET) break;

                    set_nonblocking(cs);
                    Client* client = new Client();
                    client->socket = cs;
                    client->got_put = false;
                    client->stop_client = false;

                    uint32_t ip = ntohl(caddr.sin_addr.s_addr);
                    sprintf(client->peer, "%u.%u.%u.%u:%u",
                            (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,
                            ntohs(caddr.sin_port));

                    clients.push_back(client);
                    printf("Client connected: %s\n", client->peer);
                }
            }
            base = 1;
        }

        std::vector<int> to_remove;
        for (size_t i = 0; i < clients.size(); ++i) {
            Client* client = clients[i];
            bool alive = true;
            short revents = pollfds[base + i].revents;

            if (revents & (POLLRDNORM | POLLHUP | POLLERR)) {
                while (true) {
                    char buf[8192];
                    int received = recv(client->socket, buf, sizeof(buf), 0);

                    if (received > 0) {
                        client->recv_buf.append(buf, received);
                        if (process_input(client) < 0) {
                            alive = false;
                            break;
                        }
                        if (client->stop_client && !global_stop) {
                            global_stop = true;
                            if (!listen_closed) {
                                closesocket(listen_socket);
                                listen_closed = true;
                            }
                        }
                        if (!try_flush_send(client)) {
                            alive = false;
                            break;
                        }
                    } else if (received == 0) {
                        alive = false;
                        break;
                    } else {
                        if (WSAGetLastError() != WSAEWOULDBLOCK) alive = false;
                        break;
                    }
                }
            }

            if (alive && (revents & POLLWRNORM)) {
                if (!try_flush_send(client)) alive = false;
            }

            if (revents & POLLNVAL) {
                alive = false;
            }

            if (!alive) {
                printf("Client disconnected: %s\n", client->peer);
                closesocket(client->socket);
                to_remove.push_back((int)i);
            }
        }
        for (int i = (int)to_remove.size() - 1; i >= 0; --i) {
            int idx = to_remove[i];
            delete clients[idx];
            clients.erase(clients.begin() + idx);
        }

        if (global_stop && clients.empty()) {
            break;
        }
    }

    for (size_t i = 0; i < clients.size(); ++i) {
        closesocket(clients[i]->socket);
        delete clients[i];
    }
    if (!listen_closed) {
        closesocket(listen_socket);
    }
    if (g_outfile) fclose(g_outfile);
    WSACleanup();
    return 0;
}
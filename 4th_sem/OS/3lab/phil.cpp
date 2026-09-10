/*
запуск:
1) скомпилить в спец консоли через cl.exe или через g++
g++ phil.cpp -o phil.exe
2) запуск
phil.exe 5000(TOTAL) 500(PHIL)
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int total_time, phil_time;
DWORD start_time;

char states[5] = {'T', 'T', 'T', 'T', 'T'};

int queue[5];
int q_size = 0;

CRITICAL_SECTION cs;
HANDLE sem_manager; 
HANDLE sem_philosophers[5]; 

volatile int is_running = 1;

DWORD WINAPI manager(void* param) 
{
    while (1) 
    {
        WaitForSingleObject(sem_manager, INFINITE);

        EnterCriticalSection(&cs);
        
        if (!is_running) {
            LeaveCriticalSection(&cs);
            break;
        }
        
        for (int i = 0; i < q_size; i++) // смотрим что у нас происходит в очереди
        {
            int id = queue[i];
            int left_neighbour = (id + 4) % 5; 
            int right_neighbour = (id + 1) % 5; 

            if (states[left_neighbour] != 'E' && states[right_neighbour] != 'E') // если соседи не едят то можно дать вилки
            {
                states[id] = 'E';
                DWORD now = GetTickCount() - start_time;
                printf("%lu:%d:T->E\n", now, id + 1);

                for (int j = i; j < q_size - 1; j++) {
                    queue[j] = queue[j + 1];
                }
                q_size--;
                i--;

                ReleaseSemaphore(sem_philosophers[id], 1, 0); // говорим философу что он может есть
            }
        }
        LeaveCriticalSection(&cs);
    }
    return 0;
}

DWORD WINAPI philosopher(void* param) 
{
    // получение индекса потока через param
    int id = (char*)param - (char*)0;

    while (1) 
    {
        Sleep(phil_time); // размышляем

        EnterCriticalSection(&cs);
        if (!is_running) {
            LeaveCriticalSection(&cs);
            break;
        }
        
        queue[q_size] = id; // просим поесть
        q_size++;
        
        ReleaseSemaphore(sem_manager, 1, 0); // зовем управляющего
        LeaveCriticalSection(&cs);

        WaitForSingleObject(sem_philosophers[id], INFINITE);

        EnterCriticalSection(&cs);
        int must_eat = 0;
        if (states[id] == 'E') {
            must_eat = 1;
        }
        LeaveCriticalSection(&cs);

        if (must_eat == 0) {
            break; 
        }

        Sleep(phil_time); // едим

        EnterCriticalSection(&cs); // заканчиваем есть
        DWORD now = GetTickCount() - start_time;
        printf("%lu:%d:E->T\n", now, id + 1);
        states[id] = 'T'; 

        ReleaseSemaphore(sem_manager, 1, 0); // зовем управляющего чтоб сказать что поели
        LeaveCriticalSection(&cs);
    }
    return 0;
}

int main(int argc, char* argv[]) 
{

    if (argc != 3) {
        printf("Usage: phil.exe TOTAL PHIL\n");
        return 1;
    }

    total_time = atoi(argv[1]);
    phil_time = atoi(argv[2]);

    InitializeCriticalSection(&cs);

    sem_manager = CreateSemaphore(NULL, 0, 100, NULL);
    for (int i = 0; i < 5; i++) {
        sem_philosophers[i] = CreateSemaphore(NULL, 0, 1, NULL);
    }

    HANDLE threads[6]; 

    start_time = GetTickCount();

    threads[5] = CreateThread(0, 0, manager, 0, 0, 0);

    for (int i = 0; i < 5; i++) {
        threads[i] = CreateThread(0, 0, philosopher, (void*)((char*)0 + i), 0, 0);
    }

    Sleep(total_time); // главный поток не используется так что его усыпляем

    EnterCriticalSection(&cs);
    is_running = 0;
    LeaveCriticalSection(&cs);

    ReleaseSemaphore(sem_manager, 1, 0); // будим управляющего чтоб он завершил
    
    for (int i = 0; i < 5; i++) {
        ReleaseSemaphore(sem_philosophers[i], 1, 0);
    }

    WaitForMultipleObjects(6, threads, TRUE, INFINITE); // ждем завершения всех потоков

    DeleteCriticalSection(&cs);
    CloseHandle(sem_manager);
    for (int i = 0; i < 5; i++) {
        CloseHandle(sem_philosophers[i]);
        CloseHandle(threads[i]);
    }
    CloseHandle(threads[5]);

    return 0;
}
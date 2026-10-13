#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TARGET_PASSWORD "SecretPass123"
#define PASSWORD_FILE "password.txt"
#define SERIAL_FILE "serial.txt"

void useful_payload() {
    double num1, num2, result;
    char operator_char;
    int run_calculator = 1;

    printf("\n=========================================\n");
    printf("[+] Calculator module started.\n");
    printf("=========================================\n\n");

    while (run_calculator) {
        printf("Enter expression in format: number operator number (e.g., 2 + 2).\n");
        printf("Enter 'q' instead of the first number to quit: ");

        if (scanf("%lf", &num1) != 1) {
            char exit_cmd;
            scanf("%c", &exit_cmd); 
            if (exit_cmd == 'q' || exit_cmd == 'Q') {
                 printf("Exiting calculator...\n");
                 run_calculator = 0;
                 continue;
            } else {
                 printf("Input error. Please enter a number or 'q' to quit.\n");
                 while ((getchar()) != '\n'); 
                 continue;
            }
        }

        if (scanf(" %c %lf", &operator_char, &num2) != 2) {
             printf("Input error for operator or second number.\n");
             while ((getchar()) != '\n');
             continue;
        }

        switch (operator_char) {
            case '+':
                result = num1 + num2;
                printf("Result: %.2lf\n\n", result);
                break;
            case '-':
                result = num1 - num2;
                printf("Result: %.2lf\n\n", result);
                break;
            case '*':
                result = num1 * num2;
                printf("Result: %.2lf\n\n", result);
                break;
            case ':':
            case '/':
                if (num2 == 0) {
                    printf("Error: Division by zero is not allowed.\n\n");
                } else {
                    result = num1 / num2;
                    printf("Result: %.2lf\n\n", result);
                }
                break;
            default:
                printf("Error: Unsupported operator '%c'.\n\n", operator_char);
                break;
        }
    }
}

// Генерация случайного серийного номера
void generate_serial(char *buffer, size_t size) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char random_chars[11];
    
    for (int i = 0; i < 10; i++) {
        random_chars[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    random_chars[10] = '\0';

    snprintf(buffer, size, "KEY$%s$", random_chars);
}

int main() {
    
    srand((unsigned int)time(NULL));

    // Ччитаем пароля из файла
    FILE *pass_file = fopen(PASSWORD_FILE, "r");
    if (pass_file == NULL) {
        printf("[-] Error: file '%s' is not found!\n", PASSWORD_FILE);
        printf("[!] Please create a file with %s name.\n", PASSWORD_FILE);
        system("pause");
        return 1;
    }

    char entered_password[256];
    if (fgets(entered_password, sizeof(entered_password), pass_file) == NULL) {
        printf("[-] Error: couldn't read password from file!\n");
        fclose(pass_file);
        system("pause");
        return 1;
    }
    fclose(pass_file);

    // Удаляем символы переноса строки
    entered_password[strcspn(entered_password, "\r\n")] = 0;

    // Проверяем пароль
    if (strcmp(entered_password, TARGET_PASSWORD) == 0) {
        printf("[+] Success! Password is correct.\n");

        // Создаем серийный номер
        char serial[32];
        generate_serial(serial, sizeof(serial));

        // Пишем его в файл
        FILE *serial_file = fopen(SERIAL_FILE, "w");
        if (serial_file != NULL) {
            fprintf(serial_file, "%s", serial);
            fclose(serial_file);
            printf("[+] Serial number was written in file '%s': %s\n", SERIAL_FILE, serial);
        } else {
            printf("[-] Error: couldn't create file '%s'!\n", SERIAL_FILE);
        }

        // Выполняем нагрузку
        useful_payload();

    } else {
        printf("[-] Error: wrong password!\n");
    }

    system("pause");
    return 0;
}
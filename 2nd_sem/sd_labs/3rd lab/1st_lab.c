//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <ctype.h>
//
//struct Replacement
//{
//    char from[100];
//    char to[100];
//};
//
//void readFiles(struct Replacement* dict, int* size, char* old_path, char* new_path)
//{
//    FILE* old_file = fopen(old_path, "r");
//    FILE* new_file = fopen(new_path, "r");
//    char old_buffer[100], new_buffer[100];
//
//    while (fgets(old_buffer, 100, old_file) != NULL)
//    {
//        old_buffer[strcspn(old_buffer, "\n")] = '\0'; // fgets считывает и '\n' в конце, необходимо его образать
//        fgets(new_buffer, 100, new_file);
//        new_buffer[strcspn(new_buffer, "\n")] = '\0';
//        strcpy((dict + *size)->from, old_buffer);
//        strcpy((dict + *size)->to, new_buffer);
//        (*size)++;
//    }
//
//    fclose(old_file);
//    fclose(new_file);
//}
//
//char* readText(char* path)
//{
//    FILE* file = fopen(path, "r");
//    fseek(file, 0, SEEK_END); // поместиться в конец файла, чтобы узнать его размер
//    long size = ftell(file); // размер файла - в байтах
//    rewind(file); // вернуться в начало
//
//    char* text = malloc(size + 1);
//    size_t read_bytes = fread(text, 1, size, file);
//    text[read_bytes] = '\0'; // так как fread возвращает и байт последнего \n, если он есть
//    fclose(file);
//
//    return text;
//}
//
//char* replaceWords(char* text, struct Replacement* dict, int dict_size)
//{
//    size_t capacity = strlen(text) * 2 + 1;
//    char* result = malloc(capacity);
//
//    char word[200]; // буффер для текущего слова
//    int wlen = 0;
//    int i = 0;
//    result[0] = '\0'; // строка пока пустая
//
//    while (text[i] != '\0')
//    {
//        if (isalnum((unsigned char)text[i]))
//        {
//            // собираем слово
//            wlen = 0;
//            while (text[i] != '\0' && isalnum((unsigned char)text[i]))
//            {
//                word[wlen++] = text[i++];
//            }
//            word[wlen] = '\0';
//
//            // поиск замены
//            const char* replace = word;
//            for (int j = 0; j < dict_size; j++)
//            {
//                if (strcmp(word, (dict + j)->from) == 0)
//                {
//                    replace = (dict + j)->to;
//                    break;
//                }
//            }
//
//            strcat(result, replace);
//        }
//        else {
//            // одиночные разделитель
//            char delim[2] = { text[i], '\0' };
//            strcat(result, delim);
//            i++;
//        }
//    }
//
//    return result;
//}
//
//void resultWrite(char* text, char* path)
//{
//    FILE* out = fopen(path, "w");
//    fputs(text, out);
//    fclose(out);
//}
//
//int countLines(char* path)
//{
//    FILE* f = fopen(path, "r");
//    int count = 0;
//    char ch = ' ';
//    while ((ch = fgetc(f)) != EOF)
//    {
//        if (ch == '\n') count++;
//    }
//    fclose(f);
//    return count;
//}
//
//int main(int argc, char* argv[])
//{
//    int dict_size = 0;
//
//    if (argc != 5)
//    {
//        printf("please, enter 5 parameters to start ...\n");
//        return 0;
//    }
//
//    char* input_text = argv[1];
//    char* old_words = argv[2];
//    char* new_words = argv[3];
//    char* result_text = argv[4];
//
//    int line_count = countLines(old_words);
//    struct Replacement* dict = malloc(line_count * sizeof(struct Replacement));
//
//    FILE* f = fopen(input_text, "w");
//    fputs("hello\tworld\nexam\t\thello", f);
//    fclose(f);
//
//    // считываем данных старых и новых слов
//    readFiles(dict, &dict_size, old_words, new_words);
//
//    // считываем исходный текст
//    char* text = readText(input_text);
//    char* res = replaceWords(text, dict, dict_size);
//
//    resultWrite(res, result_text);
//
//    free(text);
//    free(res);
//    free(dict);
//    return 0;
//}

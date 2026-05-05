#include "parser.h"
#include <string.h>

// parser выполняет работу по переводу карты с файла в массив, создает матрицу смежноости и массив регионов, в общем считает основные переменные, которые понадобятся далее

// проверка, содержится ли символ в regions[0..count-1] 
static int contains(char regions[MAX_SIZE], int count, char symbol) {
    for (int i = 0; i < count; i++) {
        if (regions[i] == symbol) return 1;
    }
    return 0;
}

// преобразование символа из файла карты в индекс региона 0..35 (0..9,A..Z) 
static int symbolToIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0'; // x - 48 обеспечивает индексацию от 0 до 9
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A'); // 10 + (x - 65) - сдвиг на 10 для индексации от 10 до 35
    return -1; //остальные символы не используем
}

// загрузка карты: удаление пробелов, заполнение map, size 
int loadMap(const char* filename, char map[MAX_SIZE][MAX_SIZE], int* size) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    /* не нужно ведь инициализируем карту еще в main
     инициализируем карту нулями - (что именно, чем, размер массива)
     memset(map, 0, sizeof(char) * MAX_SIZE * MAX_SIZE);
     */

    char line[256]; //пускай max_size = 30, но лучше перестраховаться
    int row = 0, max_col = 0;

    while (row < MAX_SIZE && fgets(line, sizeof(line), f)) { // цикл для движения по строкам
        int col = 0;
        for (int k = 0; line[k] != '\0' && line[k] != '\n'; k++) { // цикл для движения вперед по строке (по столбцам)
            unsigned char ch = (unsigned char)line[k];
            if (!isspace(ch) && col < MAX_SIZE) { // доп проверка а то вдруг пробелы между символами
                map[row][col++] = (char)ch;
            }
        }
        if (col > max_col) max_col = col; // тут же узнаем максимальный столбец (при обходе первой же строки)
        if (col > 0) row++; // двигаемся на следующую строку
    }

    fclose(f);

    *size = (row > max_col ? row : max_col); // в случае если наша карта вдруг не квадратная мы все равно достроим ее до квадратной узнав самый далеко выходящий ряд/столбец
    if (*size > MAX_SIZE) *size = MAX_SIZE; // проверка на аномалию, все что больше 30 - не берем в рассчет
    return 1;
}

// формирование списка регионов в порядке 0-9, A-Z а так же счет сколько регионов всего на карте
int countRegions(char map[MAX_SIZE][MAX_SIZE], int size, char regions[MAX_SIZE]) {
    int seen[36] = { 0 }; // 0..9,A..Z
    int count = 0;
    // в этом цикле проходим по карте и записываем в массив seen какие символы мы встретили
    for (int r = 0; r < size; r++) { // row - ряд
        for (int c = 0; c < size; c++) { //сolumn - столбец
            char ch = map[r][c];
            if (ch == 0) continue;
            int idx = symbolToIndex(ch);
            if (idx >= 0 && idx < 36) seen[idx] = 1; // обозначаем что данный символ присутствует на карте
        }
    }

    // сначала цифры
    for (int d = 0; d <= 9; d++) {
        if (seen[d]) regions[count++] = '0' + d; // заполняем массив regions символами которые мы встретили, теперь в четком порядке - сначала 0-9 и потом A-Z
    }
    // затем буквы
    for (int a = 0; a < 26; a++) {
        if (seen[10 + a]) regions[count++] = 'A' + a; 
    }
    return count;
} // в результате массив regions содержит в себе ровно count элементов, которые отсортированы в порядке увеличения их ASCII кода и индексированы по этому же принципу

// получение индекса региона по символу 
static int getRegionIndex(char symbol, char regions[MAX_SIZE], int region_count) {
    for (int i = 0; i < region_count; i++) {
        if (regions[i] == symbol) return i;
    }
    return -1;
}

// построение матрицы смежности: диагональ для цвета, остальные — смежность 
void buildConMatrix(char map[MAX_SIZE][MAX_SIZE], int size,
    int matrix[MAX_SIZE][MAX_SIZE],
    char regions[MAX_SIZE],
    int region_count) {
    // инициализация матрицы - по началу никакой регион с другим не соединен
    for (int i = 0; i < region_count; i++) {
        for (int j = 0; j < region_count; j++) {
            if (i == j) {
                matrix[i][i] = COLOR_NONE; // цвет еще не задан
            }
            else {
                matrix[i][j] = CON_NO; // никакие регионы еще не соединены
            }
        }
    }

    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            char cur = map[r][c]; // дальше идет два вида проверки есть регион на карте или нет
            if (cur == 0) continue; // первая
            int idx = getRegionIndex(cur, regions, region_count); // берем индекс сейчашнего символа и его тоже проверяем
            if (idx == -1) continue; // вторая: -1 значит не встречался на карте - пропускаем

            // проверка 4-соседей
            int dr[4] = { -1, 1, 0, 0 }; // dr - direction row
            int dc[4] = { 0, 0, -1, 1 }; // dc - direction column
            for (int k = 0; k < 4; k++) {
                int nr = r + dr[k], nc = c + dc[k]; // nr и nc: new row и new col (или можно neighbour) - соседи которых мы проверяем, при: k = 0 - верхний, = 1 - нижний, = 2 - левый, = 3 - правый
                if (nr >= 0 && nr < size && nc >= 0 && nc < size) { // проверка можем ли мы проверить текущего соседа - если он выходит за пределы карты как вперед так и назад - то идем дальше
                    char nb = map[nr][nc]; // сохраняем на будущее символ соседа
                    if (nb != 0 && nb != cur) { // сравниваем, если сосед - не граница карты и не совпадает с текущим символов, то
                        int nidx = getRegionIndex(nb, regions, region_count); // проверяем есть ли наш сосед на карте
                        if (nidx != -1) {
                            matrix[idx][nidx] = CON_YES; // если да то говорим что наш регион и сосед соединены, сохраняем значение соединенности зеркально 
                            matrix[nidx][idx] = CON_YES;
                        }
                    }
                }
            }
        }
    }
}

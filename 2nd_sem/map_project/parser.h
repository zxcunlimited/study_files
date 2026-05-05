#ifndef PARSER_H //используется для предотвращения ошибки повторного включения файла (к примеру если include parser.h и include файл в котором уже произошел include parser.h то ошибки не будет)
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_SIZE 30

// перечисление для соседства регионов
typedef enum {
    CON_NO = 0,  // регионы не соседствуют
    CON_YES = 1  // регионы соседствуют
} ConnectedState;

// перечисление для цветов регионов, которые будут записываться в arr[i][i]
typedef enum {
    COLOR_NONE = 0,
    COLOR_RED = 2, // начинаем с 2 чтобы случайно не перпутать с соседством
    COLOR_GREEN = 3,
    COLOR_BLUE = 4,
    COLOR_YELLOW = 5
} Color;

/* загрузка карты из файла.
   map — выходной массив MAX_SIZE x MAX_SIZE (заполняются только существующие ячейки, остальные 0)
   size — выходной размер карты (size x size)
   возвращает 1 при успехе
*/
int loadMap(const char* filename, char map[MAX_SIZE][MAX_SIZE], int* size);

/* формирование списка уникальных регионов в порядке 0-9, A-Z
   regions — выходной массив символов
   возвращает количество регионов 
*/
int countRegions(char map[MAX_SIZE][MAX_SIZE], int size, char regions[MAX_SIZE]);

/* построение матрицы смежности (целочисленная матрица).
     - matrix[i][j] == CON_YES / CON_NO для i != j (смежность)
     - matrix[i][i] == COLOR_* (COLOR_NONE, COLOR_RED, ...) — цвет региона
   используем тип int для матрицы, чтобы диагональ могла хранить коды COLOR_*. */
void buildConMatrix(char map[MAX_SIZE][MAX_SIZE], int size,
    int matrix[MAX_SIZE][MAX_SIZE],
    char regions[MAX_SIZE],
    int region_count);

#endif 

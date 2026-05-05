#include "graph.h"

// graph выполняет backtracking раскраску матрицы смежности

// возможные цвета для перебор
static const enum Color colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW
};
static const int COLOR_COUNT = 4;

/*
  проверяет, можно ли назначить вершине v цвет col,
  исходя из текущего состояния матрицы matrix.
 */
int isSafe(int v, int matrix[MAX_SIZE][MAX_SIZE], int region_count, enum Color col) { // здесь v - индекс в матрице смежности, ровно как и j
    for (int j = 0; j < region_count; j++) {
        if (j == v) continue;

        // если регионы v и j — соседи
        if (matrix[v][j] == (int)CON_YES) {
            // и сосед уже имеет тот же цвет — нельзя
            if ((enum Color)matrix[j][j] == col) {
                return 0;
            }
        }
    }
    return 1; // безопасно
}

// реализация backtracking-алгоритма раскраски.
int colorGraph(int v, int matrix[MAX_SIZE][MAX_SIZE], int region_count) { // здесь v - индекс (или номер) в матрице смежности (в списке регионов)
    // базовый случай — все вершины окрашены
    if (v >= region_count) {
        return 1;
    }

    // если цвет уже задан (к примеру этот регион раньше рассматривали как соседа), пропускаем вершину
    if (matrix[v][v] != COLOR_NONE) {
        return colorGraph(v + 1, matrix, region_count); //идем дальше по алгоритму проверять другую вершину
    }

    // пробуем все четыре цвета
    for (int i = 0; i < COLOR_COUNT; i++) {
        enum Color col = colors[i]; // берем i-ый цвет и пробуем раскрасить нашу вершину в него

        if (isSafe(v, matrix, region_count, col)) { // если никакой сосед не имеет того же цвета col
            // присваиваем цвет
            matrix[v][v] = (int)col;

            // рекурсивно красим следующую вершину
            if (colorGraph(v + 1, matrix, region_count)) {
                return 1;
            }
            // если следующую вершину не получилось окрасить в данный цвет, мы возвращаем обратно в статус нераскрашенной, пропускаем весь for и делаем return 0: откат (backtracking)
            matrix[v][v] = (int)COLOR_NONE;
        }
    }

    // если ни один цвет не подходит
    return 0;
}

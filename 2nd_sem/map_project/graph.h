#ifndef GRAPH_H
#define GRAPH_H

#include "parser.h" // понадобится для MAX_SIZE, enum ConnectedState и colors

/*
  проверяет, можно ли присвоить вершине v цвет color
  возвращает 1, если безопасно (нет конфликтов с соседями),
  иначе 0.
 */
int isSafe(int v, int matrix[MAX_SIZE][MAX_SIZE], int region_count, enum Color col);

/*
  выполняет раскраску графа методом backtracking.
  на диагонали matrix[i][i] записываются цвета регионов.
  возвращает 1, если раскраска найдена, иначе 0.
 */
int colorGraph(int v, int matrix[MAX_SIZE][MAX_SIZE], int region_count);

#endif 

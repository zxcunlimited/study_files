#ifndef RENDER_H
#define RENDER_H

#include <GL/freeglut.h>
#include "parser.h"   // MAX_SIZE, enum Color
#include "graph.h"    // colorGraph

/*
  инициализаци€ GLUT и запуск главного цикла визуализации.
  map Ч исходна€ карта 
  matrix Ч матрица смежности/цветов
  map_size - размер карты, понадобитс€ дл€ масштабировани€
  region_count Ч число регионов
  regions Ч массив символов регионов в том пор€дке, в котором были нумерованы 
 */
void renderInit(char map[MAX_SIZE][MAX_SIZE],
    int matrix[MAX_SIZE][MAX_SIZE],
    int region_count,
    int map_size,
    char regions[MAX_SIZE]);

#endif 

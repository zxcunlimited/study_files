#include <stdio.h>
#include <locale.h>
#include "parser.h"
#include "graph.h"
#include "render.h"


int main(int argc, char* argv[]) {

    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    char map[MAX_SIZE][MAX_SIZE] = { 0 }; // двумерный массив 30х30 - карта
    int matrix[MAX_SIZE][MAX_SIZE] = { 0 }; // двумерный массив 30х30 - матрица смежности
    char regions[MAX_SIZE] = { 0 }; // одномерный массив регоинов - можно сказать что линейное представление картицы смежности

    int map_size = 0;
    int region_count = 0;

    const char *filename = "map.txt"; // задаем через указатель чтобы если вдруг вызывали через консоль то можно было изменить
    if (argc > 1) {
        filename = argv[1]; 
    }

    // 1) загрузить карту и получить её размер 
    if (!loadMap(filename, map, &map_size)) {
        fprintf(stderr, "Ошибка: не удалось прочитать карту из файла %s\n", filename);
        return 1;
    }

    // 2) найти уникальные регионы (в порядке 0..9, A..Z) 
    region_count = countRegions(map, map_size, regions);
    if (region_count <= 0) {
        fprintf(stderr, "Ошибка: не найдено регионов в карте\n");
        return 1;
    }

    // 3) построить матрицу смежности (matrix - int matrix, диагональ = COLOR_NONE) 
    buildConMatrix(map, map_size, matrix, regions, region_count);

    // 4) запустить алгоритм раскраски 
    if (!colorGraph(0, matrix, region_count)) {
        fprintf(stderr, "Предупреждение: colorGraph не нашёл раскраску (вернул 0).\n");
        // продолжаем, чтобы увидеть результат в окне 
    }
    else {
        printf("colorGraph: раскраска найдена.\n");
    }

    // 5) запустить визуализацию (передаём также regions и region_count) 
    renderInit(map, matrix, map_size, region_count, regions);

    return 0;
}

#include "render.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>

// render отвечает за визуальную составляющую - работу с glut для отрисовки меню и самой карты
/* небольшие пояснения для работы с glut
существуют три вида матриц: проекции (система координат), вида и моделей (положение объектов в окне), первая определяет как 3d-координаты (у них есть не только система счисления в 2d но и глубина) проецируются на 2d экран
проблема для нашего случая в том что мы работаем одновременно с двумя системами координат и поэтому нам надо будет переключаться между матрицами 
*/

/* глобальные данные для GLUT
   так как GLUT требует чтобы функции обратного (callback) вызова (такие как наша отрисовка display) 
   не имели аргументов, нам приходится объявлять глобальные переменные
*/
static char (*g_map)[MAX_SIZE];
static int (*g_matrix)[MAX_SIZE];
static int g_map_size = 0;
static int g_region_count = 0;;
static char g_regions[MAX_SIZE];

// размеры окна при вызове
static int g_window_w = 600;
static int g_window_h = 600;

// квадратный viewport для карты (в пикселях относительно окна) 
static int g_vx = 0;
static int g_vy = 0;
static int g_vsize = 0;

typedef struct Button{
    int x, y, w, h;
    int id; 
    const char* label; // надпись на кнопке (на англ)
}Button ;

#define BUTTON_COUNT 3
static Button g_buttons[BUTTON_COUNT]; // сразу инициализируем массив кнопок

// флаг: 0 - не раскрашена, 1 - раскрашена
static int g_colored = 0;

// стандартный размер нашего окна
static const int DEFAULT_WINDOW = 600;

// состояние приложения: сначала объявляем перечисление а потом сразу же присуждаем значение - должно быть только меню
typedef enum { STATE_MENU, STATE_MAP } AppState;
static AppState g_state = STATE_MENU;

// вспомогательная: получить индекс региона по символу, используя g_regions[] 
static int getRegionIndexFromRegions(char symbol) {
    for (int i = 0; i < g_region_count; ++i) {
        if (g_regions[i] == symbol) return i;
    }
    return -1;
}

// преобразование цвета
static void setColor(enum Color c) {
    switch (c) {
    case COLOR_RED:     glColor3f(1.0f, 0.0f, 0.0f); break;
    case COLOR_GREEN:   glColor3f(0.0f, 1.0f, 0.0f); break;
    case COLOR_BLUE:    glColor3f(0.0f, 0.0f, 1.0f); break;
    case COLOR_YELLOW:  glColor3f(1.0f, 1.0f, 0.0f); break;
    default:            glColor3f(0.8f, 0.8f, 0.8f); break; // нераскрашенные - серые
    }
}

// отрисовка текста в пикселях
static void drawText2D(int x, int y, const char* text) {
    glRasterPos2i(x, y); // эта функция задает точку с которой начнет рисоваться первый пиксель символа
    for (const unsigned char* p = (const unsigned char*)text; *p; p++) { // проходим отдельно по каждому символу и пишем его
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p); // синтаксис (font, char) рисует символ и сам сдвигает дальше позицию для рисования следующего символа
    }
}

/* отрисовать кнопку (прямоугольник + текст) в пикселях */
static void drawButton(const Button* b) {
    // сначала рисуем темно серый прямоугольник
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS); // начало рисования четырехугольника с левого нижнего угла, при том важен порядок вершин - строго по/против часовой
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
    // теперь рисуем его границы черным
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
    // теперь отрисовываем текст, центрируя его по высоте и чуть сдвинув от границы
    int tx = b->x + 8; 
    int ty = b->y + (b->h / 2) - 6; // -6 чтобы текст писался не с середины а чуть выше нижней границы кнопки (при том h = 28)
    drawText2D(tx, ty, b->label); // ВАЖНО: рисует текст того цвета который ранее вызван, в нашем случае - черный
}

// проверка попадания в кнопку в координатах окна (пиксели) 
static int hitButton(int mx, int my) {
    // примечание: изначально glut считает y с верхнего левого угла, но мы делаем это с нижнего левого (преобразование происходит перед вызовом функции)
    for (int i = 0; i < BUTTON_COUNT; i++) { // проходим по всем кнопкам и проверяем
        Button* b = &g_buttons[i];
        if (mx >= b->x && mx <= b->x + b->w && // если координата нажатия в пределах от начала (x) до конца кнопки (x + width)
            my >= b->y && my <= b->y + b->h) { // если координата нажатия в пределах от начала (y) до конца кнопки (y + height)
            return b->id; // то возвращаем id кнопки которую нажали
        }
    }
    return 0; // иначе - ничего не нажали
}

// рисование одной ячейки
static void drawCell(int i, int j, float cellSize) { 
    char region = g_map[i][j]; // берем символ, соответствующий индексам из карты
    if (region == 0) return; // пустая ячейка

    // определяем индекс региона так же как и в render.c 
    int regionIndex = getRegionIndexFromRegions(region);
    enum Color color = COLOR_NONE;
    // после получения индекса региона берем его цвет с диагонали
    if (regionIndex >= 0 && regionIndex < g_region_count) { // проверяем что индекс корректный
        color = (enum Color)g_matrix[regionIndex][regionIndex]; // и берем цвет с диагонали
    }

    setColor(color); // устанавливаем текущий цвет которым будем рисовать
    // система координат все такая же - в нижнем левом углу начало координат
    float x = j * cellSize; // j - индекс ячейки по x слева-направо
    float y = (g_map_size - i - 1) * cellSize; // i - индекс ячейки сверху вниз (потому что двумерный массив строится вниз-вправо), так что мы инвертируем ось Y, потому что рисуем вниз

    glBegin(GL_QUADS); // внутри begin и end рисуем замкнутый прямоугольник (GL_QUADS - группа по 4 вершины)
    glVertex2f(x, y); // здесь и далее передаем координаты вершин по которым будем рисовать
    glVertex2f(x + cellSize, y);
    glVertex2f(x + cellSize, y + cellSize);
    glVertex2f(x, y + cellSize);
    glEnd();

    // контур
    glColor3f(0.0f, 0.0f, 0.0f); // контур полностью черный
    glBegin(GL_LINE_LOOP); // рисуем замкнутуную линию по тем же координатам 
    glVertex2f(x, y);
    glVertex2f(x + cellSize, y);
    glVertex2f(x + cellSize, y + cellSize);
    glVertex2f(x, y + cellSize);
    glEnd();

}


// основная функция отрисовки 
static void display(void) {
    
    glClear(GL_COLOR_BUFFER_BIT); // очищаем буфер

    if (g_state == STATE_MENU) 
    {

        // overlay: рисуем в координатах окна (пикселях) 
        // сохраним текущий viewport (тот, в котором рисовалась карта)
        GLint prev_vp[4]; // массив для хранения 4 данных типа GLint
        glGetIntegerv(GL_VIEWPORT, prev_vp); // записываем параметры текущего viewport'a - сначала левый нижний угол (x, y) а после ширина и высота (w, h)

        // переключим viewport на весь размер окна, чтобы координаты кнопок
        // (которые считаются в пикселях окна) соответствовали месту отрисовки
        glViewport(0, 0, g_window_w, g_window_h); // теперь мы рисуем во всем окне а не только в квадрате, предназначенном для карты

        // настраиваем систему координат для кнопок 
        glMatrixMode(GL_PROJECTION); // переключение на матрицу проекции (в данный момент в ней матрица карты)
        glPushMatrix(); // кладем КОПИЮ текущей матрицу проекции в стек (сохраняем матрицу карты чтобы потом если что с ней работать)
        glLoadIdentity(); // сбрасывает текущую матрицу в единичную = убираем все предыдущие преобразования
        glOrtho(0, g_window_w, 0, g_window_h, -1, 1); // устанавливаем координаты в пикселях (на ВСЕ окно, так как положение кнопок зависит от размера окна)

        // рисуем кнопки
        for (int i = 0; i < BUTTON_COUNT; i++) drawButton(&g_buttons[i]);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        // восстановим прежний viewport (обратно в квадрат для карты) 
        glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);

    }

    else
        {

        //glMatrixMode(GL_MODELVIEW); // переходим на модельный тип матрицы 
        //glLoadIdentity(); 

        float cellSize = (g_map_size > 0) ? 1.0f : 0.0f; // просто проверка (а то вдруг кривая карта) чтобы верно все рисовать, защита от нулевого размера

        for (int i = 0; i < g_map_size; i++) {
            for (int j = 0; j < g_map_size; j++) {
                if (g_map[i][j] != 0) {
                    drawCell(i, j, cellSize); // рисуем все ячейки карты
                }
            }
        }

        // overlay: рисуем в координатах окна (пикселях) 
        // сохраним текущий viewport (тот, в котором рисовалась карта)
        GLint prev_vp[4];
        glGetIntegerv(GL_VIEWPORT, prev_vp);

        // переключим viewport на весь размер окна, чтобы координаты кнопок
        // (которые считаются в пикселях окна) соответствовали месту отрисовки
        glViewport(0, 0, g_window_w, g_window_h);


        // делаем оверлей: переключаемся в пиксельные координаты для рисования кнопок
        glMatrixMode(GL_PROJECTION); // таким образом на данный момент наша матрица: настроена на весь оконный viewport и является пискельной
        glPushMatrix(); // кладем получаенную нами матрицу
        glLoadIdentity(); 
        glOrtho(0, g_window_w, 0, g_window_h, -1, 1); // теперь делаем матрицу для отрисовки

        // теперь тоже рисусем кнопки но уже с картой
        for (int i = 0; i < BUTTON_COUNT; i++) drawButton(&g_buttons[i]);

        // восстанавливаем матрицы
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        // восстановим прежний viewport (обратно в квадрат для карты) 
        glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);

    }

    glutSwapBuffers(); // меняем местами буферы чтобы вывести отрисованное изображение
}

static void buildButtonsLayout(void); // определение функции постройки кнопок для корректной работы функции 

// reshapre: вызывается при любой операции с окном (изменение размера) - пересчитывает размеры окна и отображение всех его составных частей
static void reshape(int w, int h) {
    g_window_w = w; // сохраняем новые размеры окна
    g_window_h = h;

    int size = (w < h) ? w : h; // используем минимальную сторону для перестроения квадратного viewport'а
    int vx = (w - size) / 2; // вычисляем новые координаты чтобы центрировать квадрат карты по центру
    int vy = (h - size) / 2;
    g_vx = vx; // сохраняем новые значения глобально 
    g_vy = vy;
    g_vsize = size;
    glViewport(vx, vy, size, size); // устанавливаем новый viewport 

    glMatrixMode(GL_PROJECTION); // переключаемся на матрицу проекции
    glLoadIdentity(); // обнуляем ее так как надо перестроить карту
    if (g_map_size > 0) {
        glOrtho(0.0, (GLdouble)g_map_size, 0.0, (GLdouble)g_map_size, -1.0, 1.0); // переустанавлиаем glortho, так как у нас изменились размеры окна (иначе он бы работал по старым размерам)
    }
    else {
        glOrtho(0.0, 2.0, 0.0, 2.0, -1.0, 1.0); // минимальный вариант размеров карты - 2х2 
    }

    // пересчитываем расположение кнопок под новый размер окна 
    buildButtonsLayout();
}

//обработчик клавиатуры (ESC = выход) 
static void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y; // заглушка для предотвращения предупреждения о неиспользованных переменных
    if (key == 27) { // ESC
        exit(0);
    }
}

// обработчик мыши (в том числе и просто ее движения)
static void mouseHandler(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return; // так как нам нужно обработать только нажатие на меню то все что кроме этого мы пропускаем
    // переводим стандартный для глута y отсчитываемый сверху-слева в y отсчитываемый снизу-слева
    int my = g_window_h - y; 
    int mx = x;

    // проверяем нажатие кнопки
    int bid = hitButton(mx, my);
    if (bid == 0) return; // если не попали то выходим

    // действуем в зависимости от нажатой кнопки
    if (bid == 1) { // 1 - раскраска карты
        if (!g_colored) {
            int ok = colorGraph(0, g_matrix, g_region_count);
            if (ok) {
                g_colored = 1;
                printf("Карта успешно раскрашена.\n");
            }
            else {
                g_colored = 0;
                printf("Не удалось раскрасить карту четырьмя цветами.\n");
            }
        }
        // меняем на статус - сейчас карта и выводим нарисованную карту
        g_state = STATE_MAP;
        glutPostRedisplay(); // просим глут перерисовать 
    }
    else if (bid == 2) { // 2 - редактировать карту - открываем файл и закрываем программу
#ifdef _WIN32
        system("start \"\" \"map.txt\"");
#else
        system("xdg-open map.txt 2>/dev/null &");
#endif
        exit(0);
    }
    else if (bid == 3) { // 3 - выход, заканчиваем работу программы
        exit(0);
    }
}

// рассчет позиций кнопок в зависимости от размера окна
static void buildButtonsLayout(void) {
    int bw = 120, bh = 28; // размеры кнопок
    int margin = 10; // расстояние между кнопками
    // отмечаем стартовые точки для кнопок - сверху-слева

    // g_vx - global viewport x, g_vsize - global viewport size, тут мы рассчитываем откуда должны начинать ставить кнопки и какое расстояние между картой и краем экрана
    int right_panel_x = g_vx + g_vsize + margin;
    int right_panel_width = g_window_w - right_panel_x - margin;

    int start_x, start_y; // координаты с которых мы будем рисовать кнопки, двигаться будем вниз  

    if (right_panel_width >= bw + margin) { // если ширина правой панели >= ширина кнопки + расстоние между ними то мы "закрепляем" кнопки правее от карты
        // ставим кнопки в правой панели, чуть ниже верхней грани окна и правее карты
        start_x = right_panel_x;
        start_y = g_window_h - margin - bh;
    }
    else {
        // иначе — размещаем в правом верхнем углу окна (как запасной вариант), при том меню будет наслаиваться на карту
        start_x = g_window_w - margin - bw;
        start_y = g_window_h - margin - bh;
    }
    // присуждаем каждой кнопке ее координаты, размеры и текст
    for (int i = 0; i < 3; i++) {
        g_buttons[i].x = start_x;
        g_buttons[i].y = start_y - (i * (bh + 6)); //y все также увеличивается снизу-вверх так что вычитаем чтобы сдвигать кнопки ниже
        g_buttons[i].w = bw;
        g_buttons[i].h = bh;
        g_buttons[i].id = i + 1;
        switch (i) {
            case 0: g_buttons[i].label = "Paint"; break;
            case 1: g_buttons[i].label = "Edit map"; break;
            case 2: g_buttons[i].label = "Exit"; break;
        }
    }
}

// инициализация GLUT и запуск визуализации 
void renderInit(char map[MAX_SIZE][MAX_SIZE], // передаем для визуализации карту
    int matrix[MAX_SIZE][MAX_SIZE], // матрицу смежности чтобы раскрашивать карту
    int map_size, // размер карты
    int region_count,  // кол-во регионов чтобы верно обходить
    char regions[MAX_SIZE]) {

    g_map = map;
    g_matrix = matrix;
    g_map_size = map_size;
    g_region_count = region_count;
    g_colored = 0;

    // копируем regions локально 
    memset(g_regions, 0, sizeof(g_regions));
    if (regions != NULL && region_count > 0) { // если с regions все нормально то заполняем массив глобальный
        int n = (region_count > MAX_SIZE) ? MAX_SIZE : region_count; // если не дай бог регионов как то больше чем размер карты то мы все равно работаем с максимальным размером
        for (int i = 0; i < n; ++i) g_regions[i] = regions[i]; // копируем данные из regions в глобальный regions
    }
    // дефолтное состояние - 600х600, меню, ничего не раскрашено
    g_window_w = DEFAULT_WINDOW;
    g_window_h = DEFAULT_WINDOW;
    g_state = STATE_MENU;
    g_colored = 0;

    int argc = 1;
    char* argv[1] = { (char*)"MapProject" };
    glutInit(&argc, argv); // инициализация glut, argc и argv берутся из main (но мы их подделали немного)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // стандартная инициализация, где параметр - как будет задаваться цвет, в нашем случае это двойная буферизация И RGB 
    glutInitWindowSize(DEFAULT_WINDOW, DEFAULT_WINDOW); // параметры - width и height, в нашем случае окно квадратное
    glutCreateWindow("Теорема о 4 красках"); // созданиие самого окна, внутри const char* заголовок окна

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // установка цвета, которым будет очищаться буфер (какой цвет будет если мы попросим очистить экран), последний параметр - альфа

    /*
    так же доп пояснение про буферы и GLUT_DOUBLE - мы задали двойной буфер: передний и задний
    передний - то что мы видим на экране а задний по сути строит следующий кадр
    когда приходит время сменить мы просто используем glutSwapBuffers, что обеспечивает нам мгновенную отрисовку и смену кадра без надобности ждать пока отрисуется весь буфер и (возможно) кривой картинки
    */

    glutDisplayFunc(display); // данная функция содержит один параметр - указатель на функцию, которая будет отвечать за рисование в окне
    glutReshapeFunc(reshape); // аналогично прошлой но для изменения размеров окна
    glutKeyboardFunc(keyboard); // аналогично прошлой но для обработки нажатий
    glutMouseFunc(mouseHandler); // аналогично но для обработки мыши

    buildButtonsLayout();

    glutMainLoop();
}

/* чтоб тестить:
2) в wsl:
g++ msort.cpp -o msort -D_REENTRANT -lpthread
3) запустить
./msort
4) можно вывести через cat output.txt/time.txt
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define SEQ_THRESHOLD 1000

#define ACT_DIVIDE 0
#define ACT_MERGE 1

typedef struct MergeNode {
    int left;
    int right;
    int mid;
    int children_done; // cчетчик завершенных задач дочерними узлами (0, 1 или 2)
    struct MergeNode* parent;
    struct MergeNode* left_child;
    struct MergeNode* right_child;
} MergeNode;

typedef struct {
    MergeNode* node;
    int action; // ACT_DIVIDE или ACT_MERGE
} QueueItem;

int threads_cnt;
int n;
int *a;
int *temp_a; // общий массив для слияния, чтобы не выделять его каждый раз 

QueueItem *queue;
int queue_capacity;
int head = 0;
int tail = 0;
int queue_count = 0;

pthread_mutex_t queue_mutex;
pthread_cond_t queue_cv; // условная переменная для ожидания появления задач
pthread_cond_t done_cv; // условная переменная для ожидания окончания всей сортировки
int shutdown_flag = 0;

void merge(int left, int mid, int right) { // слияние двух отсортированных частей массива
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        if (a[i] <= a[j]) temp_a[k++] = a[i++];
        else temp_a[k++] = a[j++];
    }
    while (i <= mid) temp_a[k++] = a[i++];
    while (j <= right) temp_a[k++] = a[j++];
    for (i = left; i <= right; i++) a[i] = temp_a[i]; // копируем обратно в основной массив
}

void seq_merge_sort(int left, int right) { // полная функция сортировки 
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    seq_merge_sort(left, mid);
    seq_merge_sort(mid + 1, right);
    merge(left, mid, right); 
}

MergeNode* create_node(int left, int right, MergeNode* parent) { // функция для создания нового узла
    MergeNode* node = (MergeNode*)malloc(sizeof(MergeNode));
    node->left = left;
    node->right = right;
    node->mid = 0;
    node->children_done = 0;
    node->parent = parent;
    node->left_child = NULL;
    node->right_child = NULL;
    return node;
}

void push_queue(MergeNode* node, int action) {
    queue[tail].node = node;
    queue[tail].action = action;
    tail = (tail + 1) % queue_capacity;
    queue_count++;
}

void trigger_parent(MergeNode* node) { // функция чтоб уведомить родителя о завершении задачи
    if (node->parent == NULL) { // попали в корень => вся сортировка завершена
        pthread_mutex_lock(&queue_mutex);
        shutdown_flag = 1;
        pthread_cond_broadcast(&queue_cv); // будим все потоки для выхода
        pthread_cond_signal(&done_cv); // будим функцию main
        pthread_mutex_unlock(&queue_mutex);
    } 
    else { // а иначе уведомляем родителя, что одна из его задач завершилась
        pthread_mutex_lock(&queue_mutex);
        node->parent->children_done++;
        int done = node->parent->children_done;
        
        if (done == 2) {
            // done = 2 => оба ребенка закончили и можно мерджить
            push_queue(node->parent, ACT_MERGE);
            pthread_cond_signal(&queue_cv); 
        }
        pthread_mutex_unlock(&queue_mutex);
    }
}

void* worker_thread(void* arg) 
{
    while (1) 
    {        
        pthread_mutex_lock(&queue_mutex);
        while (queue_count == 0 && !shutdown_flag) {
            pthread_cond_wait(&queue_cv, &queue_mutex); 
        }
        
        if (shutdown_flag && queue_count == 0) {
            pthread_mutex_unlock(&queue_mutex);
            break; 
        }
        
        QueueItem item = queue[head]; // извлекаем задачу из очереди
        head = (head + 1) % queue_capacity;
        queue_count--;
        pthread_mutex_unlock(&queue_mutex);

        MergeNode* node = item.node; // извлекаем узел из задачи

        if (item.action == ACT_DIVIDE) 
        {
            if (node->right - node->left <= SEQ_THRESHOLD) { // ровно как и в qsortе, если массив небольшой то можно и самому его отсортировать 
                seq_merge_sort(node->left, node->right);
                trigger_parent(node);
            } 
            else {
                node->mid = node->left + (node->right - node->left) / 2;
                
                MergeNode* l_child = create_node(node->left, node->mid, node);
                MergeNode* r_child = create_node(node->mid + 1, node->right, node);
                node->left_child = l_child;
                node->right_child = r_child;

                pthread_mutex_lock(&queue_mutex);
                push_queue(l_child, ACT_DIVIDE);
                push_queue(r_child, ACT_DIVIDE);
                pthread_cond_broadcast(&queue_cv); // будим все потоки чтобы они могли взять новые задачи
                pthread_mutex_unlock(&queue_mutex);
            }
        }
        else if (item.action == ACT_MERGE) 
        {
            merge(node->left, node->mid, node->right);

            free(node->left_child);
            free(node->right_child);
            
            trigger_parent(node);
        }
    }
    return NULL;
}

unsigned long get_ms(struct timespec* tm) {
    return (unsigned long)(tm->tv_sec * 1000 + tm->tv_nsec / 1000000);
}

int main() 
{
    FILE* fin = fopen("input.txt", "r");
    fscanf(fin, "%d", &threads_cnt);
    fscanf(fin, "%d", &n);

    a = (int*)malloc(n * sizeof(int));
    temp_a = (int*)malloc(n * sizeof(int)); 
    
    for (int i = 0; i < n; i++) {
        fscanf(fin, "%d", &a[i]);
    }
    fclose(fin);

    // размер очереди с запасом, чтоб не тратить время на malloc
    queue_capacity = n + 10000; 
    queue = (QueueItem*)malloc(queue_capacity * sizeof(QueueItem));

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cv, NULL); // монитор для очереди
    pthread_cond_init(&done_cv, NULL); // монитор для отслеживания завершения задач

    pthread_t* threads = (pthread_t*)malloc(threads_cnt * sizeof(pthread_t));
    for (int i = 0; i < threads_cnt; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    struct timespec tm_start, tm_end;
    clock_gettime(CLOCK_REALTIME, &tm_start);

    MergeNode* root = create_node(0, n - 1, NULL); // самый первый узел дерева
    
    pthread_mutex_lock(&queue_mutex);
    push_queue(root, ACT_DIVIDE);
    pthread_cond_signal(&queue_cv);

    while (!shutdown_flag) {
        pthread_cond_wait(&done_cv, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    for (int i = 0; i < threads_cnt; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &tm_end);
    unsigned long elapsed_time = get_ms(&tm_end) - get_ms(&tm_start);

    FILE* fout = fopen("output.txt", "w");
    fprintf(fout, "%d\n%d\n", threads_cnt, n);
    for (int i = 0; i < n; i++) {
        fprintf(fout, "%d ", a[i]);
    }
    fprintf(fout, "\n");
    fclose(fout);

    FILE* ftime = fopen("time.txt", "w");
    fprintf(ftime, "%lu\n", elapsed_time);
    fclose(ftime);

    free(root);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cv);
    pthread_cond_destroy(&done_cv);
    free(threads);
    free(queue);
    free(a);
    free(temp_a);

    return 0;
}
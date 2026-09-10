/* чтоб тестить:
2) в wsl:
g++ qsort.cpp -o qsort -D_REENTRANT -lpthread
3) запустить
./qsort
4) можно вывести через cat output.txt
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define SEQ_THRESHOLD 1000 // потоку выгоднее сортировать самому массив меньше 1000 элементов, чем создавать новые потоки, так что это порог, при котором лучше не создавать новые потоки

// границы подмассива для сортировки
typedef struct {
    int left;
    int right;
} Task;

int threads_cnt;
int n;
int *a;

// очередь задач
Task *queue;
int queue_capacity;
int head = 0;
int tail = 0;
int queue_count = 0;

sem_t sem_tasks; // семафоро том сколько задач сейчас в очереди
sem_t sem_queue_lock; // семафор об эксклюзивном доступе к очереди

int working_threads = 0; 
int shutdown_flag = 0;

void seq_qsort(int left, int right) 
{
    if (left >= right) return;
    
    int i = left;
    int j = right;
    int pivot = a[left + (right - left) / 2];
    
    while (i <= j) {
        while (a[i] < pivot) i++;
        while (a[j] > pivot) j--;
        if (i <= j) {
            int temp = a[i];
            a[i] = a[j];
            a[j] = temp;
            i++;
            j--;
        }
    }
    
    if (left < j) seq_qsort(left, j);
    if (i < right) seq_qsort(i, right);
}

void push_task(int left, int right) 
{
    queue[tail].left = left;
    queue[tail].right = right;
    tail = (tail + 1) % queue_capacity;
    queue_count++;
}

Task pop_task() 
{
    Task t = queue[head];
    head = (head + 1) % queue_capacity;
    queue_count--;
    return t;
}
\
void* thread_entry(void* arg) 
{
    while (1) 
    {
        // спим пока нет задач
        sem_wait(&sem_tasks);
        
        sem_wait(&sem_queue_lock); // захватываем очередь
        
        if (shutdown_flag) {
            sem_post(&sem_queue_lock);
            break; 
        }
        
        Task task = pop_task();
        working_threads++; 
        
        sem_post(&sem_queue_lock); // освобождаем очередь

        if (task.right - task.left <= SEQ_THRESHOLD) {
            seq_qsort(task.left, task.right);
        } 
        else {
            int i = task.left;
            int j = task.right;
            int pivot = a[task.left + (task.right - task.left) / 2];
            
            while (i <= j) {
                while (a[i] < pivot) i++;
                while (a[j] > pivot) j--;
                if (i <= j) {
                    int temp = a[i];
                    a[i] = a[j];
                    a[j] = temp;
                    i++;
                    j--;
                }
            }
            
            // добавление новых задач
            sem_wait(&sem_queue_lock);
            
            if (task.left < j) {
                push_task(task.left, j);
                sem_post(&sem_tasks); // +1 задача
            }
            if (i < task.right) {
                push_task(i, task.right);
                sem_post(&sem_tasks); // +1 задача
            }
            
            sem_post(&sem_queue_lock);
        }

        // захватываем чтоб проверить завершение
        sem_wait(&sem_queue_lock);
        working_threads--;
        
        if (queue_count == 0 && working_threads == 0) {
            shutdown_flag = 1;
            for (int k = 0; k < threads_cnt; k++) { // будим все спящие потоки чтобы заверишилсь
                sem_post(&sem_tasks);
            }
        }
        sem_post(&sem_queue_lock);
    }
    
    return NULL;
}

unsigned long get_ms(struct timespec* tm) 
{
    return (unsigned long)(tm->tv_sec * 1000 + tm->tv_nsec / 1000000);
}

int main() 
{
    FILE* fin = fopen("input.txt", "r");
    fscanf(fin, "%d", &threads_cnt);
    fscanf(fin, "%d", &n);

    a = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        fscanf(fin, "%d", &a[i]);
    }
    fclose(fin);

    queue_capacity = n + 1000;
    queue = (Task*)malloc(queue_capacity * sizeof(Task));

    sem_init(&sem_tasks, 0, 0); // семафор задач (готовых к тому, чтобы их можно было обработать)
    sem_init(&sem_queue_lock, 0, 1); // семафор блокировки очереди - по сути тот же мьютекс

    pthread_t* threads = (pthread_t*)malloc(threads_cnt * sizeof(pthread_t));
    for (int i = 0; i < threads_cnt; i++) {
        pthread_create(&threads[i], NULL, thread_entry, NULL);
    }

    struct timespec tm_start, tm_end;
    clock_gettime(CLOCK_REALTIME, &tm_start);

    sem_wait(&sem_queue_lock); // захватываем очередь чтоб добавить первую и основную задачу
    push_task(0, n - 1);
    sem_post(&sem_tasks); 
    sem_post(&sem_queue_lock);

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

    sem_destroy(&sem_tasks);
    sem_destroy(&sem_queue_lock);
    free(threads);
    free(queue);
    free(a);

    return 0;
}
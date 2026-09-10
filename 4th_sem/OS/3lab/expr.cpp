#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int threads_cnt;
int n;
long long s;
long long a1;

// данные левой половины 
int n_left;
long long *left_arr;
long long *left_double;
long long left_base_sum = 0;
unsigned long long left_count;
long long *left_sums; // массив всех возможных сумм левой половины

// данные правой половины 
int n_right;
long long *right_arr;
long long *right_double;
long long right_base_sum = 0;

// общие переменные
unsigned long long total_tasks;
volatile unsigned long long current_task = 0;
volatile unsigned long long solutions_count = 0;

HANDLE mutex;
HANDLE start_event;

// для встроенной qsort
int compare_ll(const void* a, const void* b) 
{
    long long arg1 = *(const long long*)a;
    long long arg2 = *(const long long*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// обычный бинарный поиск
unsigned long long lower_bound(long long* arr, unsigned long long size, long long val) 
{
    unsigned long long left = 0, right = size;
    while (left < right) {
        unsigned long long mid = left + (right - left) / 2;
        if (arr[mid] < val) left = mid + 1;
        else right = mid;
    }
    return left;
}

// бинарный поиск но первое число, которое СТРОГО БОЛЬШЕ искомого
unsigned long long upper_bound(long long* arr, unsigned long long size, long long val) 
{
    unsigned long long left = 0, right = size;
    while (left < right) {
        unsigned long long mid = left + (right - left) / 2;
        if (arr[mid] <= val) left = mid + 1;
        else right = mid;
    }
    return left;
}

DWORD WINAPI thread_entry(void* param) 
{
    unsigned long long local_sols = 0;
    unsigned long long chunk = 10000;
    
    if (total_tasks < chunk * threads_cnt) {
        chunk = total_tasks / threads_cnt;
        if (chunk == 0) chunk = 1;
    }

    WaitForSingleObject(start_event, INFINITE);

    while (1) 
    {
        unsigned long long start, end;

        // захватываем кусок задач из правой половины
        WaitForSingleObject(mutex, INFINITE);
        if (current_task >= total_tasks) {
            ReleaseMutex(mutex);
            break;
        }
        start = current_task;
        end = start + chunk;
        if (end > total_tasks) end = total_tasks; 
        current_task = end;
        ReleaseMutex(mutex);

        for (unsigned long long mask = start; mask < end; mask++) 
        {
            long long right_sum = right_base_sum;
            unsigned long long t = mask;
            int i = 0;
            
            while (t > 0) {
                if (t & 1) right_sum -= right_double[i];
                t >>= 1;
                i++;
            }
            
            // итоговая формула это a1 + leftS + rightS = S, значит ищем LeftS = S - a1 - RightS
            long long target = s - a1 - right_sum;
            
            unsigned long long first = lower_bound(left_sums, left_count, target);
            
            // теперь так как массив левых сумм отсортирован, то мы просто ищем первый индекс числа которое больше найденного target
            if (first < left_count && left_sums[first] == target) {
                unsigned long long last = upper_bound(left_sums, left_count, target);
                local_sols += (last - first);
            }
        }
    }

    if (local_sols > 0) {
        WaitForSingleObject(mutex, INFINITE);
        solutions_count += local_sols;
        ReleaseMutex(mutex);
    }

    return 0;
}

int main() 
{
    FILE* in = fopen("input.txt", "r");
    fscanf(in, "%d", &threads_cnt);
    fscanf(in, "%d", &n);
    // а1 отдельно потому что оно всегда без знака
    fscanf(in, "%lld", &a1);
    
    n_left = (n - 1) / 2;
    n_right = (n - 1) - n_left;

    left_arr = (long long*)malloc(n_left * sizeof(long long));
    left_double = (long long*)malloc(n_left * sizeof(long long));
    for (int i = 0; i < n_left; i++) {
        fscanf(in, "%lld", &left_arr[i]);
        left_base_sum += left_arr[i];
        left_double[i] = left_arr[i] * 2;
    }

    right_arr = (long long*)malloc(n_right * sizeof(long long));
    right_double = (long long*)malloc(n_right * sizeof(long long));
    for (int i = 0; i < n_right; i++) {
        fscanf(in, "%lld", &right_arr[i]);
        right_base_sum += right_arr[i];
        right_double[i] = right_arr[i] * 2;
    }
    
    fscanf(in, "%lld", &s);
    fclose(in);

    left_count = 1 << n_left; // побитовый сдвиг позволяет быстро вычислить 2^n_left, потому что именно столько различных комбинаций элементов может быть
    left_sums = (long long*)malloc(left_count * sizeof(long long));
    total_tasks = 1 << n_right;

    start_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    mutex = CreateMutex(NULL, FALSE, NULL);

    HANDLE* threads = (HANDLE*)malloc(threads_cnt * sizeof(HANDLE));
    for (int i = 0; i < threads_cnt; i++) {
        threads[i] = CreateThread(0, 0, thread_entry, 0, 0, 0);
    }

    // начало замера времени
    DWORD t_start = GetTickCount();

    // строим массив левых сумм
    for (unsigned long long mask = 0; mask < left_count; mask++) {
        long long current_left = left_base_sum;
        unsigned long long t = mask;
        int i = 0;
        while (t > 0) {
            if (t & 1) current_left -= left_double[i];
            t >>= 1;
            i++;
        }
        left_sums[mask] = current_left;
    }

    // сортируем массив левых сумм для бинарного поиска
    qsort(left_sums, left_count, sizeof(long long), compare_ll);

    SetEvent(start_event);

    for (int i = 0; i < threads_cnt; i++) {
        WaitForSingleObject(threads[i], INFINITE); 
    }
    // конец замера времени
    DWORD t_end = GetTickCount();
    

    FILE* out = fopen("output.txt", "w");
    fprintf(out, "%d\n%d\n%llu\n", threads_cnt, n, solutions_count);
    fclose(out);

    FILE* time_out = fopen("time.txt", "w");
    fprintf(time_out, "%lu\n", t_end - t_start);
    fclose(time_out);

    for (int i = 0; i < threads_cnt; i++) {
        CloseHandle(threads[i]);
    }
    CloseHandle(mutex);
    CloseHandle(start_event);
    
    free(threads);
    free(left_arr); free(left_double); free(left_sums);
    free(right_arr); free(right_double);

    return 0;
}
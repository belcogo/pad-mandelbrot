#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define LARGURA 800
#define ALTURA 800
#define MAX_ITER 256
#define NUM_THREADS 2

// Define structures
typedef struct {
    int x_inicio, x_fim;
    int y_inicio, y_fim;
} ThreadData;

typedef struct {
    int x, y;
    int color;
} ItemToPrint;

// Global variables


/// Work Buffer
ThreadData *buffer_work_to_be_done;
int current_index_from_workers_buffer = 0;
int last_index_from_workers_buffer = 0;
pthread_mutex_t mutex_work_to_be_done;
pthread_cond_t cond_work_to_be_done;

/// Print buffer 
ItemToPrint *colors_to_be_printed;
int current_index_from_printer_buffer = 0;
int last_index_from_printer_buffer = 0;
pthread_mutex_t mutex_work_to_be_printed;
pthread_cond_t cond_work_to_be_printed;

SDL_Renderer *renderer;
SDL_mutex *sdl_mutex;

double xmin = -2.0, xmax = 1.0;
double ymin = -1.5, ymax = 1.5;

bool has_data_to_be_proceeded() {
    return last_index_from_workers_buffer > current_index_from_workers_buffer;
}

bool has_data_to_be_printed() {
    return last_index_from_printer_buffer > current_index_from_printer_buffer;
}

int mandelbrot(double real, double imag) {
    double z_real = 0.0, z_imag = 0.0;
    int n = 0;
    while (z_real * z_real + z_imag * z_imag <= 4.0 && n < MAX_ITER) {
        double temp_real = z_real * z_real - z_imag * z_imag + real;
        z_imag = 2.0 * z_real * z_imag + imag;
        z_real = temp_real;
        n++;
    }
    return n;
}

void* worker(void *arg) {
    while (true) {
        pthread_mutex_lock(&mutex_work_to_be_done);

        while (!has_data_to_be_proceeded()) {
            pthread_cond_wait(&cond_work_to_be_done, &mutex_work_to_be_done);
        }

        int index_to_consider = current_index_from_workers_buffer;
        current_index_from_workers_buffer++;

        pthread_mutex_unlock(&mutex_work_to_be_done);

        ThreadData data_to_procced = buffer_work_to_be_done[index_to_consider];
        ItemToPrint item_to_print;
        int size_of_temp_array = 0 * 0; // TODO: Precisamos mudar quando tivermos o controle de quadrado.
        int index_temp_array = 0;
        ItemToPrint temp_array[size_of_temp_array];

        for (int y = data->y_inicio; y < data->y_fim; y++) {
            for (int x = data->x_inicio; x < data->x_fim; x++) {
                double real = xmin + (xmax - xmin) * x / LARGURA;
                double imag = ymin + (ymax - ymin) * y / ALTURA;
                int iter = mandelbrot(real, imag);
                int color = iter % 256;

                item_to_print.x = x;
                item_to_print.y = y;
                item_to_print.color = color;

                temp_array[index_temp_array] = item_to_print;
                index_temp_array++;
            }
        }

        pthread_mutex_lock(&mutex_work_to_be_printed);

        for (int i = 0; i < size_of_temp_array; i++) {
            last_index_from_printer_buffer++;
            colors_to_be_printed[last_index_from_printer_buffer] = temp_array[index_temp_array];
        }

        pthread_mutex_unlock(&mutex_work_to_be_printed);
        pthread_cond_signal(&cond_work_to_be_printed);
    }
}

void create_workers(int quantity_of_threads) {
    for (int i = 0; i < quantity_of_threads; i++) {
        pthread_create(i, NULL, worker, NULL);
    }
}

void thread_print() {
    while (true) {
        pthread_mutex_lock(&mutex_work_to_be_printed);

        while (!has_data_to_be_proceeded()) {
            pthread_cond_wait(&cond_work_to_be_printed, &mutex_work_to_be_printed);
        }

        int index_to_consider = current_index_from_printer_buffer;
        current_index_from_printer_buffer++;

        pthread_mutex_unlock(&mutex_work_to_be_printed);

        // TODO: Fazer as chamadas para printrar quadrado.
    }
}

void* render_fractal(void *arg) {
    ThreadData *data = (ThreadData*) arg;

    for (int y = data->y_inicio; y < data->y_fim; y++) {
        for (int x = data->x_inicio; x < data->x_fim; x++) {
            double real = xmin + (xmax - xmin) * x / LARGURA;
            double imag = ymin + (ymax - ymin) * y / ALTURA;
            int iter = mandelbrot(real, imag);
            int cor = iter % 256;

            // TODO: remove render from here
            // TODO: write on result buffer
            // Lock para evitar conflitos entre threads no renderizador
            SDL_LockMutex(sdl_mutex);
            SDL_SetRenderDrawColor(renderer, cor, cor, cor, 255);
            SDL_RenderDrawPoint(renderer, x, y);
            SDL_UnlockMutex(sdl_mutex);
        }
    }
    return NULL;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Mandelbrot Paralelo (Blocos)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    sdl_mutex = SDL_CreateMutex();

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    // Determina o número de blocos em cada eixo
    int blocos_x = 2;
    int blocos_y = NUM_THREADS / blocos_x;
    int largura_bloco = LARGURA / blocos_x;
    int altura_bloco = ALTURA / blocos_y;

    int thread_id = 0;
    for (int by = 0; by < blocos_y; by++) {
        for (int bx = 0; bx < blocos_x; bx++) {
            thread_data[thread_id].x_inicio = bx * largura_bloco;
            thread_data[thread_id].x_fim = (bx == blocos_x - 1) ? LARGURA : (bx + 1) * largura_bloco;
            thread_data[thread_id].y_inicio = by * altura_bloco;
            thread_data[thread_id].y_fim = (by == blocos_y - 1) ? ALTURA : (by + 1) * altura_bloco;
            pthread_create(&threads[thread_id], NULL, render_fractal, &thread_data[thread_id]);
            thread_id++;
        }
    }

    // TODO: add while (cond_mutex) to read from result buffer
    // TODO: create thread to render
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }


    SDL_RenderPresent(renderer);
    SDL_Delay(5000);

    SDL_DestroyMutex(sdl_mutex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

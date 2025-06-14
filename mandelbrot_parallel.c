#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define LARGURA 800
#define ALTURA 800
#define MAX_ITER 256
#define NUM_THREADS 8

SDL_Renderer *renderer;
double xmin = -2.0, xmax = 1.0, ymin = -1.5, ymax = 1.5;

typedef struct {
    int thread_id;
    int y_inicio;
    int y_fim;
} ThreadData;

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

void* render_fractal(void *arg) {
    ThreadData *data = (ThreadData*) arg;

    for (int y = data->y_inicio; y < data->y_fim; y++) {
        for (int x = 0; x < LARGURA; x++) {
            double real = xmin + (xmax - xmin) * x / LARGURA;
            double imag = ymin + (ymax - ymin) * y / ALTURA;
            int iter = mandelbrot(real, imag);
            int cor = iter % 256;

            SDL_SetRenderDrawColor(renderer, cor, cor, cor, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
    return NULL;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Mandelbrot Paralelo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    int linhas_por_thread = ALTURA / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].y_inicio = i * linhas_por_thread;
        thread_data[i].y_fim = (i == NUM_THREADS - 1) ? ALTURA : (i + 1) * linhas_por_thread;
        pthread_create(&threads[i], NULL, render_fractal, &thread_data[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(10000);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

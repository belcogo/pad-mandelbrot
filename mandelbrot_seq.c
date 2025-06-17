#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#define LARGURA 800
#define ALTURA 800
#define MAX_ITER 256
#define BLOCK_SIZE 10

SDL_Renderer *renderer;

int temp_array[LARGURA][ALTURA];

// Função que verifica se o ponto c pertence ao conjunto de Mandelbrot
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

void processa(int y_inicio, int y_fim, int x_inicio, int x_fim) {
    double xmin = -2.0, xmax = 1.0, ymin = -1.5, ymax = 1.5;
    
    for (int y = y_inicio; y < y_fim; y++) {
      for (int x = x_inicio; x < x_fim; x++) {
            // Mapeando as coordenadas da tela para o plano complexo
            double real = xmin + (xmax - xmin) * x / LARGURA;
            double imag = ymin + (ymax - ymin) * y / ALTURA;

            // Calculando o número de iterações
            int iter = mandelbrot(real, imag);

            // Determinando a cor baseado no número de iterações
            int cor = (iter % 256); // Cor em tons de cinza
            temp_array[x][y] = cor;
      }
    }
}

void renderiza(int y_inicio, int y_fim, int x_inicio, int x_fim) {
    for (int y = y_inicio; y < y_fim; y++) {
      for (int x = x_inicio; x < x_fim; x++) {
            int cor = temp_array[x][y];

            // Setando a cor e desenhando o ponto na tela
            int r = (cor * 5) % 256;
            int g = (cor * 7) % 256;
            int b = (cor * 11) % 256;
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, x, y);

            // Atualizando a tela a cada ponto desenhado
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
      }
    }
}

int main() {
    clock_t inicio = clock();
    SDL_Init(SDL_INIT_VIDEO);

    // Criando a janela e a tela
    SDL_Window *window = SDL_CreateWindow("Conjunto de Mandelbrot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // Definindo os limites do plano complexo


    int blocks_x = (LARGURA + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int blocks_y = (ALTURA + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (int by = 0; by < blocks_y; by++) {
        for (int bx = 0; bx < blocks_x; bx++) {
            int x_inicio = bx * BLOCK_SIZE;
            int x_fim = ((bx + 1) * BLOCK_SIZE > LARGURA) ? LARGURA : (bx + 1) * BLOCK_SIZE;
            int y_inicio = by * BLOCK_SIZE;
            int y_fim = ((by + 1) * BLOCK_SIZE > ALTURA) ? ALTURA : (by + 1) * BLOCK_SIZE;
            processa(y_inicio, y_fim, x_inicio, x_fim);
            renderiza(y_inicio, y_fim, x_inicio, x_fim);
        }
    }


    // Gerando a imagem do conjunto de Mandelbrot de forma incremental
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {

        }
    }

    clock_t fim = clock();
    double tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Tempo total para finalizar computação e apresentação: %.7f segundos\n", tempo);

    // Aguardando um tempo para exibir a imagem antes de fechar
    SDL_Delay(5000);

    // Limpando e fechando
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
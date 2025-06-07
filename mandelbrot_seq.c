#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define LARGURA 800
#define ALTURA 800
#define MAX_ITER 256

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

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    // Criando a janela e a tela
    SDL_Window *window = SDL_CreateWindow("Conjunto de Mandelbrot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Definindo os limites do plano complexo
    double xmin = -2.0, xmax = 1.0, ymin = -1.5, ymax = 1.5;

    // Gerando a imagem do conjunto de Mandelbrot de forma incremental
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            // Mapeando as coordenadas da tela para o plano complexo
            double real = xmin + (xmax - xmin) * x / LARGURA;
            double imag = ymin + (ymax - ymin) * y / ALTURA;

            // Calculando o número de iterações
            int iter = mandelbrot(real, imag);

            // Determinando a cor baseado no número de iterações
            int cor = (iter % 256); // Cor em tons de cinza

            // Setando a cor e desenhando o ponto na tela
            SDL_SetRenderDrawColor(renderer, cor, cor, cor, 255); // Escala de cinza
            SDL_RenderDrawPoint(renderer, x, y);

            // Atualizando a tela a cada ponto desenhado
            SDL_RenderPresent(renderer);
        }
    }

    // Aguardando um tempo para exibir a imagem antes de fechar
    SDL_Delay(5000);

    // Limpando e fechando
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define WIDTH 800
#define HEIGHT 800

// Define structures
typedef struct {
  int x_inicio, x_fim;
  int y_inicio, y_fim;
} Block;

typedef struct {
    int x, y;
    int color;
} ItemToPrint;

/// Work Buffer
Block *buffer_work_to_be_done;
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

// Global variables
SDL_Renderer *renderer;

double xmin = -2.0, xmax = 1.0;
double ymin = -1.5, ymax = 1.5;

int block_size;
int total_blocks;
int max_iter;
bool application_finished = false;

bool has_data_to_process() {
  return last_index_from_workers_buffer > current_index_from_workers_buffer;
}

bool has_data_to_be_printed() {
  return last_index_from_printer_buffer > current_index_from_printer_buffer;
}

bool has_printed_everything() {
  return (WIDTH * HEIGHT) == current_index_from_printer_buffer;
}

int mandelbrot(double real, double imag) {
  double z_real = 0.0, z_imag = 0.0;
  int n = 0;
  while (z_real * z_real + z_imag * z_imag <= 4.0 && n < max_iter) {
    double temp_real = z_real * z_real - z_imag * z_imag + real;
    z_imag = 2.0 * z_real * z_imag + imag;
    z_real = temp_real;
    n++;
  }
  return n;
}

void* producer(void* arg) {
  while (true) {
    pthread_mutex_lock(&mutex_work_to_be_done);

    while (!has_data_to_process()) {
      pthread_cond_wait(&cond_work_to_be_done, &mutex_work_to_be_done);

      if (application_finished) {
        pthread_exit(NULL);
      }
    }

    int index_to_process = current_index_from_workers_buffer;
    current_index_from_workers_buffer++;

    pthread_mutex_unlock(&mutex_work_to_be_done);

    Block data_to_process = buffer_work_to_be_done[index_to_process];
    ItemToPrint item_to_print;
    int size_of_temp_array = block_size * block_size;
    int index_temp_array = 0;
    ItemToPrint temp_array[size_of_temp_array];

    for (int y = data_to_process.y_inicio; y < data_to_process.y_fim; y++) {
      for (int x = data_to_process.x_inicio; x < data_to_process.x_fim; x++) {
        double real = xmin + (xmax - xmin) * x / WIDTH;
        double imag = ymin + (ymax - ymin) * y / HEIGHT;
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
      colors_to_be_printed[last_index_from_printer_buffer] = temp_array[i];
      last_index_from_printer_buffer++;
    }

    pthread_mutex_unlock(&mutex_work_to_be_printed);
    pthread_cond_signal(&cond_work_to_be_printed);
  }
}

void* create_work_to_be_done(void* arg) {
  int block_id = 0;
  int block_size = (int)(intptr_t) arg;
  int blocks_x = (WIDTH + block_size - 1) / block_size;
  int blocks_y = (HEIGHT + block_size - 1) / block_size;

  for (int by = 0; by < blocks_y; by++) {
    for (int bx = 0; bx < blocks_x; bx++) {
      pthread_mutex_lock(&mutex_work_to_be_done);
      buffer_work_to_be_done[block_id].x_inicio = bx * block_size;
      buffer_work_to_be_done[block_id].x_fim = ((bx + 1) * block_size > WIDTH) ? WIDTH : (bx + 1) * block_size;
      buffer_work_to_be_done[block_id].y_inicio = by * block_size;
      buffer_work_to_be_done[block_id].y_fim = ((by + 1) * block_size > HEIGHT) ? HEIGHT : (by + 1) * block_size;
      
      block_id++;
      last_index_from_workers_buffer++;

      pthread_mutex_unlock(&mutex_work_to_be_done);
      pthread_cond_signal(&cond_work_to_be_done);
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  clock_t inicio = clock();

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("Mandelbrot Paralelo (Blocos)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  pthread_mutex_init(&mutex_work_to_be_done, NULL);
  pthread_cond_init(&cond_work_to_be_done, NULL);

  pthread_mutex_init(&mutex_work_to_be_printed, NULL);
  pthread_cond_init(&cond_work_to_be_printed, NULL);

  int NUM_THREADS = atoi(argv[1]);
  block_size = atoi(argv[2]);
  max_iter = atoi(argv[3]);

  printf("Executando considerando: \n- Threads: %d \n- Block Size: %d\n- Max Inter: %d \n", NUM_THREADS, block_size, max_iter);

  // Determina o número de blocos em cada eixo
  int blocks_x = (WIDTH + block_size - 1) / block_size;
  int blocks_y = (HEIGHT + block_size - 1) / block_size;
  total_blocks = blocks_x * blocks_y;

  buffer_work_to_be_done = malloc(total_blocks * sizeof(Block));
  colors_to_be_printed = malloc(WIDTH * HEIGHT * sizeof(ItemToPrint));

  // Definição de trabalhos a serem executados.
  pthread_t creation_work_thread;
  pthread_create(&creation_work_thread, NULL, create_work_to_be_done, (void*)(intptr_t) block_size);

  // Criação das threads dos publishers
  pthread_t *threads = malloc(NUM_THREADS * sizeof(pthread_t));

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, producer, NULL);
  }
  
  
  // Função para manter window executando e printar coisas na tela.
  while (!application_finished) {
    pthread_mutex_lock(&mutex_work_to_be_printed);

    while (!has_data_to_be_printed()) {
      pthread_cond_wait(&cond_work_to_be_printed, &mutex_work_to_be_printed);
    }

    while (current_index_from_printer_buffer <= last_index_from_printer_buffer) {
        ItemToPrint item = colors_to_be_printed[current_index_from_printer_buffer];
        // int color = item.color % 256;
        int r = (item.color * 5) % 256;
        int g = (item.color * 7) % 256;
        int b = (item.color * 11) % 256;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawPoint(renderer, item.x, item.y);

        application_finished = has_printed_everything();
        current_index_from_printer_buffer++;
    }

    pthread_mutex_unlock(&mutex_work_to_be_printed);

    SDL_RenderPresent(renderer);
    SDL_Delay(16); // ~60fps

    if (application_finished) {
      break;
    }
  }

  free(threads);
  free(buffer_work_to_be_done);
  free(colors_to_be_printed);

  // Destroí variável condicional e mutex do printer.
  pthread_cond_destroy(&cond_work_to_be_printed);
  pthread_mutex_destroy(&mutex_work_to_be_printed);

  // Destroí variável condicional e mutex do producer.
  pthread_cond_broadcast(&cond_work_to_be_done);
  pthread_cond_destroy(&cond_work_to_be_done);
  pthread_mutex_destroy(&mutex_work_to_be_done);

  clock_t fim = clock();
  double tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
  printf("Tempo total para finalizar computação e apresentação: %.3f segundos\n", tempo);

  // Espero 15 segundos.
  sleep(15);
    
  // Finalizo a window.
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

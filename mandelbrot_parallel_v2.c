#include <pthread.h>
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
  int thread_id;
  int starting_block;
} ThreadData;

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

// Processed blocks
int blocks_processed = 0;
pthread_mutex_t mutex_blocks_processed;

// Global variables
SDL_Renderer *renderer;

double xmin = -2.0, xmax = 1.0;
double ymin = -1.5, ymax = 1.5;

int block_size;
int total_blocks;
int max_iter;

bool workers_done = false;
bool printer_done = false;

bool running = true;
SDL_Event event;

bool has_data_to_process() {
  return last_index_from_workers_buffer > current_index_from_workers_buffer;
}

bool has_data_to_be_printed() {
  return last_index_from_printer_buffer > current_index_from_printer_buffer;
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

void* printer(void* arg) {
  while (!printer_done) {
    int start_index, end_index;

    pthread_mutex_lock(&mutex_work_to_be_printed);

    while (!has_data_to_be_printed() && !printer_done) {
      pthread_cond_wait(&cond_work_to_be_printed, &mutex_work_to_be_printed);
    }

    if (printer_done) {
      pthread_mutex_unlock(&mutex_work_to_be_printed);
      break;
    }

    // Define intervalo dos itens novos no buffer
    start_index = current_index_from_printer_buffer;
    end_index = last_index_from_printer_buffer;

    // Apenas avança o ponteiro de leitura — a main vai consumir esses itens
    pthread_mutex_unlock(&mutex_work_to_be_printed);

    // Envia um evento para a thread principal renderizar
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = 0;  // pode usar isso se quiser distinguir tipos de eventos
    event.user.data1 = NULL;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    // Verifica se o trabalho acabou
    pthread_mutex_lock(&mutex_blocks_processed);
    if (workers_done && !has_data_to_be_printed()) {
        printer_done = true;

        // Envia um último evento para garantir que a main acorde e finalize
        SDL_Event final_event;
        final_event.type = SDL_USEREVENT;
        final_event.user.code = 1;
        final_event.user.data1 = NULL;
        final_event.user.data2 = NULL;
        SDL_PushEvent(&final_event);
    }
    pthread_mutex_unlock(&mutex_blocks_processed);

    pthread_cond_signal(&cond_work_to_be_printed);
  }
}

void* producer(void* arg) {
  ThreadData *thread_data = (ThreadData *) arg;
  while(!workers_done) {
    pthread_mutex_lock(&mutex_work_to_be_done);

    while (!has_data_to_process() && !workers_done) {
      pthread_cond_wait(&cond_work_to_be_done, &mutex_work_to_be_done);
    }

    if (workers_done) {
      pthread_mutex_unlock(&mutex_work_to_be_done);
      break;  // Exit loop and end thread
    }

    Block data_to_process = buffer_work_to_be_done[current_index_from_workers_buffer];
    current_index_from_workers_buffer++;
    ItemToPrint item_to_print;
    int size_of_temp_array = block_size * block_size; // TODO: Precisamos mudar quando tivermos o controle de quadrado.
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

    pthread_mutex_lock(&mutex_blocks_processed);
    blocks_processed++;
    if (blocks_processed == total_blocks) {
        workers_done = true;
    }
    pthread_mutex_unlock(&mutex_blocks_processed);

    pthread_mutex_unlock(&mutex_work_to_be_done);
    pthread_cond_signal(&cond_work_to_be_done);

    pthread_mutex_lock(&mutex_work_to_be_printed);
    for (int i = 0; i < size_of_temp_array; i++) {
      colors_to_be_printed[last_index_from_printer_buffer] = temp_array[i];
      last_index_from_printer_buffer++;
    }

    pthread_mutex_unlock(&mutex_work_to_be_printed);
    pthread_cond_signal(&cond_work_to_be_printed);
  }
}

int main(int argc, char *argv[]) {
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

  pthread_mutex_init(&mutex_blocks_processed, NULL);


  int NUM_THREADS = atoi(argv[1]);
  block_size = atoi(argv[2]);
  max_iter = atoi(argv[3]);

  // Determina o número de blocos em cada eixo
  int blocks_x = (WIDTH + block_size - 1) / block_size;
  int blocks_y = (HEIGHT + block_size - 1) / block_size;
  total_blocks = blocks_x * blocks_y;
  last_index_from_workers_buffer = total_blocks;
  buffer_work_to_be_done = malloc(total_blocks * sizeof(Block));
  colors_to_be_printed = malloc(WIDTH * HEIGHT * sizeof(ItemToPrint));

  int block_id = 0;
  for (int by = 0; by < blocks_y; by++) {
    for (int bx = 0; bx < blocks_x; bx++) {
      buffer_work_to_be_done[block_id].x_inicio = bx * block_size;
      buffer_work_to_be_done[block_id].x_fim = ((bx + 1) * block_size > WIDTH) ? WIDTH : (bx + 1) * block_size;
      buffer_work_to_be_done[block_id].y_inicio = by * block_size;
      buffer_work_to_be_done[block_id].y_fim = ((by + 1) * block_size > HEIGHT) ? HEIGHT : (by + 1) * block_size;
      block_id++;
    }
  }


  pthread_t *threads = malloc(NUM_THREADS * sizeof(pthread_t));
  ThreadData *thread_data = malloc(NUM_THREADS * sizeof(ThreadData));

  int blocks_per_thread = total_blocks / NUM_THREADS;
  int remainder = total_blocks % NUM_THREADS;
  int start = 0;

  for (int i = 0; i < NUM_THREADS; i++) {
    int count = blocks_per_thread + (i < remainder ? 1 : 0);
    thread_data[i].thread_id = i;
    thread_data[i].starting_block = start;
    start += count;

    pthread_create(&threads[i], NULL, producer, &thread_data[i]);
  }
  pthread_t print_thread;
  pthread_create(&print_thread, NULL, printer, NULL);

  while (running) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
            break;
        } else if (event.type == SDL_USEREVENT) {
          if (event.user.code == 1) {
              running = false;  // Finaliza quando o trabalho todo acabar
              break;
          }
      }
    }

    pthread_mutex_lock(&mutex_work_to_be_printed);

    while (current_index_from_printer_buffer < last_index_from_printer_buffer) {
        ItemToPrint item = colors_to_be_printed[current_index_from_printer_buffer++];
        // int color = item.color % 256;
        int r = (item.color * 5) % 256;
        int g = (item.color * 7) % 256;
        int b = (item.color * 11) % 256;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawPoint(renderer, item.x, item.y);
    }

    pthread_mutex_unlock(&mutex_work_to_be_printed);

    SDL_RenderPresent(renderer);
    SDL_Delay(16); // ~60fps
}

  for (int i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], NULL);
  }

  pthread_join(print_thread, NULL);


  free(thread_data);
  free(threads);
  free(buffer_work_to_be_done);
  free(colors_to_be_printed);

  // SDL_Delay(5000);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

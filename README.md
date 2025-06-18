# Trabalho de Programação de Alto Desempenho - Mandelbrot

Nomes: Bel Cogo e Bruno da Siqueira Hoffmann

## Pré requisitos
1. Lib gcc
2. SDL/SLD2

## Instalando o SDL
- Execute os passos presentes no arquivo: https://wiki.libsdl.org/SDL2/Installation.

## Como rodar o programa
1. Na raiz do projeto execute o comando `gcc -o mandelbrot_parallel_v3 mandelbrot_parallel_v3.c $(sdl2-config --cflags --libs)`
2. Execute o comando `./mandelbrot_parallel_v3 <numero-threads> <tamanho-bloco> <complexidade>`. Exemplo: `./mandelbrot_parallel_v3 2 10 100`.

## Dados Obtidos nas Execuções:

- Experimento feito em uma máquina de 16GB RAM, i5-1334U - 12 threads, Ubuntu 22.04;

- Cenário 1:

| Inter | Threads** | Quadrados | Tempo    |
|-------|-----------|-----------|----------|
| 65536 | 1         | 100       | 39.149s  |
| 65536 | 2         | 100       | 39.312s  |
| 65536 | 3         | 100       | 45.391s  |
| 65536 | 5         | 100       | 52.109s  |

- Cenário 2:

| Inter | Threads** | Quadrados | Tempo    |
|-------|-----------|-----------|----------|
| 65536 | 1         | 10        | 39.340s  |
| 65536 | 2         | 10        | 39.715s  |
| 65536 | 3         | 10        | 45.988s  |
| 65536 | 5         | 10        | 54.268s  |

- Cenário 3:

| Inter | Threads**   | Tempo   | Quadrados |
|-------|-------------|---------|-----------|
| 256   | Sequencial  | 24 mins |   10      |
| 256   | 1 thread    | 0,688s  |   10      |
| 256   | 2 threads   | 0,721s  |   10      |
| 256   | 5 threads   | 0,680s  |   10      |

Notas:
- Threads**: O número de threads do producer, ainda há o número de threads (main e orquestrador de trabalho);

### Conclusões

Inicialmente, houve uma grande redução do tempo de execução ao paralelizar o código, onde ele saiu da casa de minutos para a casa de segundos. Mas, com o aumento de threads foi observado um aumento no tempo, que após uma análise do código possivelmente pode estar associado com o modo que está sendo feito a adição dos quadros para imprimir dentro do buffer, onde a thread de producer fica muito tempo com o mutex da sessão crítica, o que provavelmente gera mais threads esperando o seu momento de entrar na sessão crítica.
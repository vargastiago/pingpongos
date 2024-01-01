// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include "queue.h"    // biblioteca de filas genéricas
#include <ucontext.h> // biblioteca POSIX de trocas de contexto

#define STACKSIZE 32768  // tamanho da pilha de threads
#define AGING_FACTOR -1  // fator de envelhecimento
#define MIN_PRIORITY 20  // prioridade mínima
#define MAX_PRIORITY -20 // prioridade máxima
#define TICKS 20         // quantum

// tipo enumerado que define os possíveis valores para o status da tarefa
typedef enum { NEW,
               READY,
               RUNNING,
               FINISHED } status_t;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t {
    struct task_t *prev, *next; // ponteiros para usar em filas
    int id;                     // identificador da tarefa
    ucontext_t context;         // contexto armazenado da tarefa
    status_t status;            // status da tarefa
    int static_prio;            // prioridade estática
    int dynamic_prio;           // prioridade dinâmica
    int is_sys_task;            // flag de tarefa do sistema
    int quantum;                // total de ticks do relógio
    int activations;            // contador de ativações
    unsigned int exec_start;    // tempo de início de execução da tarefa
    unsigned int exec_end;      // tempo de término de execução da tarefa
    unsigned int proc_marker;   // marcador de tempo parcial de processamento
    unsigned int proc_time;     // tempo total de processamento
} task_t;

// estrutura que define um semáforo
typedef struct
{
    // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
    // preencher quando necessário
} mqueue_t;

#endif

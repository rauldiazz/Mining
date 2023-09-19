/**
 * @file monitor.h
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo monitor.h, cabecera con la declaracion de funciones y estructuras de monitor.c.
 * @version 0.1
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>

#define OK 0
#define MAX_WORKERS 10
#define PRIME 99997669
#define BIG_X 435679812
#define BIG_Y 100001819
#define SHM_NAME_NET "/netdata"
#define SHM_NAME_BLOCK "/block"
#define MQ_MONITOR "/mq_monitor"
#define SIZE 10

#define MAX_MINERS 200

typedef struct _Block
{
    int wallets[MAX_MINERS];
    char voting_pool[MAX_MINERS];
    long int target;
    long int solution;
    int id;
    int is_valid;
    struct _Block *next;
    struct _Block *prev;
} Block;

typedef struct _NetData
{
    pid_t miners_pid[MAX_MINERS];
    int miners_join[MAX_MINERS];
    int last_miner;
    int salidas;
    int active_miners;
    int total_miners;
    pid_t monitor_pid;
    pid_t last_winner;
    sem_t aux;
    sem_t bloque_escrito;
    sem_t bloque_recibido;
    sem_t cont;
    sem_t iniciar_votacion;
    sem_t contar_votos;
    sem_t sem_mutex;
    sem_t sem_block;
    sem_t winner;
} NetData;

typedef struct
{
    long int target;
    long int start, end;
    long int solution;
} worker_struct;

void manejadorSigAlarm(int sig);
void manejadorSigInt(int sig);

void print_blocks(Block *plast_block, int num_wallets, FILE *pf);
int add_to_chain(Block **chain, int *wallets, long int target, long int solution, int id, int is_valid);
void establecer_manejadores();
void liberar_recursos(Block *blockChain, NetData *struc_net, mqd_t mq_monitor);
int esperar_semaforo(sem_t *sem);
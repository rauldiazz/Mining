/**
 * @file miner.h
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo miner.h, cabecera con la declaracion de funciones y estructuras de miner.c.
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
#include <semaphore.h>
#include <sys/mman.h>
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
long int simple_hash(long int number);
void print_blocks(Block *plast_block, int num_wallets, FILE *pf);
Block *initializeBlocks(NetData *struc_net, mqd_t *mq_monitor);
NetData *attachToNet(int *num_minero);
long int simple_hash(long int number);
int esperar_semaforo(sem_t *sem);
void *Busqueda(void *arg);
long int inicializarHilos(pthread_t *hilos, NetData *struc_net, worker_struct *strucs, Block *struc_block, int num_workers);
void send_sigusr2(NetData *struc_net);
void reservasMalloc(pthread_t **hilos, worker_struct **strucs, int num_workers);
pthread_t *mallocHilos(int num_workers);
worker_struct *mallocWorker(int num_workers);
int add_to_chain(Block **chain, int *wallets, long int target, long int solution, int id, int is_valid);
/*Funciones de sincronización*/
int escritura_solucion(NetData *struc_net, Block *struc_block, long int solution);
int iniciar_votacion(NetData *struc_net, Block *struc_block);
int esperar_votacion(NetData *struc_net);
int consultar_votacion(NetData *struc_net, Block *struc_block, int num_minero);
int actualizacion_bloque(NetData *struc_net, Block *struc_block, Block **blockChain, mqd_t mq_monitor);
int realizar_votacion(NetData *struc_net, Block *struc_block, int num_minero);
int actualizar_cadena(NetData *struc_net, Block *struc_block, Block **blockChain, mqd_t mq_monitor);
int establecer_manejadores();
void manejadorSigUsr2(int sig);
void manejadorSigInt(int sig);
void liberar_recursos(pthread_t *hilos, NetData *struc_net, Block *struc_block, worker_struct *strucs, mqd_t mq_monitor, int num_minero, Block *blockChain);
int inicio_ronda(int *flag_salida, NetData *struc_net, int num_minero, volatile int sig_int, int num_rounds, int m);

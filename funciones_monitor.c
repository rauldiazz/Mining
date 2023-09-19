/**
 * @file funciones_monitor.c
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo auxiliar funciones_monitor.c, que contiene la implementación de varias funciones declaradas en monitor.h que se utilizarán en monitor.c.
 * @version 0.1
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "monitor.h"

/**
 * @brief Funcion que imprime los bloques en un fichero de texto.
 * 
 * @param plast_block ultimo bloque de la lista enlazada de bloques.
 * @param num_wallets numero de wallets.
 * @param pf fichero en el que se va a escribir.
 */
void print_blocks(Block *plast_block, int num_wallets, FILE *pf)
{
    Block *block = NULL;
    int i, j;

    for (i = 0, block = plast_block; block != NULL; block = block->prev, i++)
    {
        fprintf(pf, "Block number: %d; Target: %ld;    Solution: %ld\n", block->id, block->target, block->solution);
        for (j = 0; j < num_wallets; j++)
        {
            fprintf(pf, "%d: %d;         ", j, block->wallets[j]);
        }
        fprintf(pf, "\n\n\n");
    }
    fprintf(pf, "A total of %d blocks were printed\n", i);
}

/**
 * @brief Funcion que añade un nuevo bloque a la cadena de bloques.
 * 
 * @param chain cadena de bloques.
 * @param wallets array con las wallets de cada minero.
 * @param target numero del que se busca la solucion.
 * @param solution solucion del problema.
 * @param id id del bloque.
 * @param is_valid resultado de la votacion.
 * @return int 0 si se ejecuta correctamente, -1 en caso de error.
 */
int add_to_chain(Block **chain, int *wallets, long int target, long int solution, int id, int is_valid)
{
    Block *block;
    int l;
    if (chain == NULL || wallets == NULL)
        return -1;
    block = (Block *)malloc(sizeof(Block));
    if (block == NULL)
    {
        perror("malloc:");
        return -1;
    }

    for (l = 0; l < MAX_MINERS; l++)
    {
        block->wallets[l] = wallets[l];
    }

    block->target = target;
    block->solution = solution;
    block->id = id;
    block->is_valid = is_valid;

    if ((*chain) == NULL) /*Caso en el que la cadena esta vacia*/
    {
        if (block == NULL)
        {
        }
        (*chain) = block;
        block->next = NULL;
        block->prev = NULL;
    }
    else /*Caso en el que la cadena no esta vacia*/
    {

        (*chain)->next = block;
        block->prev = (*chain);
        (*chain) = block;
    }
    return 0;
}

/**
 * @brief Funcion que establece los manejadores de la señal SIGINT y SIGALARM del monitor.
 * 
 * @return int 0 si se ejecuta correctamente, -1 si hay error.
 */
void establecer_manejadores()
{

    struct sigaction act_sigint, act_sigalarm;
    act_sigint.sa_handler = manejadorSigInt;
    sigfillset(&(act_sigint.sa_mask));
    act_sigint.sa_flags = 0;

    act_sigalarm.sa_handler = manejadorSigAlarm;
    sigfillset(&(act_sigalarm.sa_mask));
    act_sigalarm.sa_flags = 0;

    if (sigaction(SIGINT, &act_sigint, NULL) < 0)
    {
        perror("sigaction: ");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGALRM, &act_sigalarm, NULL) < 0)
    {
        perror("sigaction: ");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Funcion que libera los recursos del monitor.
 * 
 * @param blockChain cadena de bloques del minero.
 * @param struc_net puntero a estructura de la red.
 * @param mq_monitor cola de mensajes con el monitor.
 * 
 */
void liberar_recursos(Block *blockChain, NetData *struc_net, mqd_t mq_monitor)
{
    esperar_semaforo(&struc_net->sem_mutex);
    struc_net->monitor_pid = -1;
    sem_post(&struc_net->sem_mutex);
    munmap(struc_net, sizeof(Block));

    Block *block, *block2;
    block = blockChain;
    while (block != NULL)
    {
        block2 = block;
        block = block->prev;
        free(block2);
    }
    mq_close(mq_monitor);
}
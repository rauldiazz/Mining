/**
 * @file miner.c
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo miner.c. Minado de bloques para el sistema de blockChain propuesto en el enunciado del proyecto.
 * @version 0.1
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "miner.h"

static volatile sig_atomic_t sig_int = 0;
static volatile sig_atomic_t sig_usr2 = 0;
static volatile sig_atomic_t sig_alarm = 0;

/**
 * @brief Funcion que gestiona la recepcion de la señal SIGINT.
 * 
 * @param sig no se usa.
 */
void manejadorSigInt(int sig)
{
    sig_int = 1;
    printf("sig_int signal received\n");
}

/**
 * @brief Funcion que gestiona la recepcion de la señal SIGUSR2.
 * 
 * @param sig no se usa.
 */
void manejadorSigUsr2(int sig)
{
    sig_usr2 = 1;
}

/**
 * @brief Funcion que gestiona la recepcion de la señal SIGALARM.
 * 
 * @param sig no se usa.
 */
void manejadorSigAlarm(int sig)
{
    sig_alarm = 1;
    printf("sig_alarm signal received\n");
}

/**
 * @brief Funcion auxiliar que realiza la espera a un semaforo con un timeout de 5 segundos.
 * 
 * @param sem semaforo al que se hace down.
 * @return int  1 si se ejecuta correctamente, -1 si hay error.
 */
int esperar_semaforo(sem_t *sem)
{
    alarm(5);
    while (sem_wait(sem) == -1)
    {
        if (errno == EINTR && sig_alarm == 1)
        {
            return -1;
        }
    }
    alarm(0);
    return 1;
}

/**
 * @brief Funcion de busqueda de la solucion al problema, asociada a los hilos
 * 
 * @param arg estructura del bloque de tipo void*.
 * @return void* devuelve NULL (no se usa el retorno)
 */
void *Busqueda(void *arg)
{
    long int l;
    worker_struct *struc;
    if (arg != NULL)
    {
        struc = arg;
    }
    else
    {
        return NULL;
    }
    for (l = struc->start; l < struc->end; l++)
    {
        if (sig_usr2 == 1)
        {
            return NULL;
        }
        if (struc->target == simple_hash(l))
        {
            if (sig_usr2 == 1)
            {
                return NULL;
            }

            struc->solution = l;
            return NULL;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int num_workers = -1, num_rounds = -1, num_minero = -1, m = 0, flag = 0, ret = -1, flag_error = 0, flag_salida = 0;
    long int solution = -1;
    pthread_t *hilos = NULL;
    pid_t pid_miner = -1;
    NetData *struc_net = NULL;
    worker_struct *strucs = NULL;
    Block *blockChain = NULL, *struc_block = NULL;
    mqd_t mq_monitor = -1;
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <num_workers> <num_rounds>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /*Obtención del pid del minero*/
    pid_miner = getpid();
    printf("Pid Miner: %jd\n", (intmax_t)pid_miner);

    num_rounds = atoi(argv[2]);
    num_workers = atoi(argv[1]);

    if (num_workers <= 0)
    {
        printf("The number of workers must be > 0");
        exit(EXIT_FAILURE);
    }

    /*El minero se queda esperando a que se reciba la señal de terminación*/
    if (num_rounds <= 0)
    {
        flag = 1;
    }
    ret = establecer_manejadores();
    if (ret == -1)
    {
        exit(EXIT_FAILURE);
    }

    /* Añadimos al minero a la red de mineros */
    struc_net = attachToNet(&num_minero);
    printf("%d", num_minero);
    if (struc_net == NULL || struc_net == MAP_FAILED)
    {
        shm_unlink(SHM_NAME_NET);
        exit(EXIT_FAILURE);
    }

    /* Inicializamos los bloques del minero */
    struc_block = initializeBlocks(struc_net, &mq_monitor);
    if (struc_block == NULL || struc_block == MAP_FAILED)
    {
        shm_unlink(SHM_NAME_NET);
        shm_unlink(SHM_NAME_BLOCK);
        exit(EXIT_FAILURE);
    }

    /* Reserva de memoria de los hilos y la estructura worker_struct */
    hilos = mallocHilos(num_workers);
    if (hilos == NULL)
    {
        munmap(struc_net, sizeof(NetData));
        munmap(struc_block, sizeof(Block));
        shm_unlink(SHM_NAME_NET);
        shm_unlink(SHM_NAME_BLOCK);
        exit(EXIT_FAILURE);
    }
    strucs = mallocWorker(num_workers);
    if (strucs == NULL)
    {
        free(hilos);
        munmap(struc_net, sizeof(NetData));
        munmap(struc_block, sizeof(Block));
        shm_unlink(SHM_NAME_NET);
        shm_unlink(SHM_NAME_BLOCK);
        exit(EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }

    /*********************************************************************Inicio del bucle principal de ejecución**************************************************************************/
    printf("Número de minero: %d   PID: %jd\n", num_minero, (intmax_t)struc_net->miners_pid[num_minero]);
    for (m = 0; m < num_rounds || flag == 1; m++)
    {

        ret = inicio_ronda(&flag_salida, struc_net, num_minero, sig_int, num_rounds, m);
        if (ret == -1)
        {
            flag_error = 1;
            break;
        }

        solution = inicializarHilos(hilos, struc_net, strucs, struc_block, num_workers);
        if (solution == -2)
        {
            flag_error = 1;
            break;
        }

        if (esperar_semaforo(&struc_net->winner) == -1)
        {
            flag_error = 1;
            break;
        }

        if (sig_usr2 == 0) /*Primeras acciones minero ganador*/
        {
            /*Escritura de la solución*/
            if (escritura_solucion(struc_net, struc_block, solution) == -1)
            {
                flag_error = 1;
                break;
            }

            /*Iniciación del proceso de votacion*/
            if (iniciar_votacion(struc_net, struc_block) == -1)
            {
                flag_error = 1;
                break;
            }
        }
        sem_post(&struc_net->winner);

        if (sig_usr2 == 0) /*últimas acciones minero ganador*/
        {
            /*esperar a la votacion*/
            if (esperar_votacion(struc_net) == -1)
            {
                flag_error = 1;
                break;
            }

            /*consultar la votacion*/
            if (consultar_votacion(struc_net, struc_block, num_minero) == -1)
            {
                flag_error = 1;
                break;
            }

            /*Tareas de actualización del bloque e inserción en la cadena del minero*/
            ret = actualizacion_bloque(struc_net, struc_block, &blockChain, mq_monitor);
            /*Permitir el inicio de una nueva ronda*/
            sem_post(&struc_net->cont);
            if (ret == -1)
            {
                flag_error = 1;
                break;
            }
            if (flag_salida == 1)
            {
                break;
            }
        }

        if (sig_usr2 == 1) /*Acciones de mineros que no han ganado la ronda*/
        {
            if (realizar_votacion(struc_net, struc_block, num_minero) == -1)
            {
                flag_error = 1;
                break;
            }

            ret = actualizar_cadena(struc_net, struc_block, &blockChain, mq_monitor);
            sig_usr2 = 0;

            if (ret == -1)
            {
                flag_error = 1;
                break;
            }
            if (flag_salida == 1)
            {
                break;
            }
        }
    }
    esperar_semaforo(&struc_net->cont);
    sem_post(&struc_net->cont);

    liberar_recursos(hilos, struc_net, struc_block, strucs, mq_monitor, num_minero, blockChain);
    if (flag_error == 1)
    {
        printf("Notificación:  Ha ocurrido un error-> Ejecución detenida\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

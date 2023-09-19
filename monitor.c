/**
 * @file monitor.c
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo monitor.c, que contiene la implementacion del monitor de la red de mineros.
 * @version 0.1
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "monitor.h"

#define SIZE_BUFFER 10

static volatile sig_atomic_t sig_int = 0;
static volatile sig_atomic_t sig_alarm = 0;

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
 * @brief Funcion que gestiona la recepcion de la señal SIGINT
 * 
 * @param sig no se usa
 */
void manejadorSigInt(int sig)
{
    sig_int = 1;
}

/**
 * @brief Funcion que gestiona la recepcion de la señal SIGINT
 * 
 * @param sig no se usa
 */
void manejadorSigAlarm(int sig)
{
    sig_alarm = 1;
}

int main(int argc, char *argv[])
{
    int pipe_status = -1, fd[2], nbytes = 0, prim = 0, flag = 0, l = 0, fd_shm = 0;
    int *wallets = NULL;

    NetData *struc_net = NULL;
    FILE *pf = NULL;

    Block bloque_recibido, buffer[10];
    Block *blockChain = NULL, *aux = NULL, *prev = NULL;

    size_t len = sizeof(Block);
    mqd_t monitor_queue = -1;
    pid_t pid = -1, pid_main = -1;

    struct mq_attr attributes =
        {
            .mq_flags = 0,
            .mq_maxmsg = 10,
            .mq_msgsize = sizeof(Block),
            .mq_curmsgs = 0};

    establecer_manejadores();

    /*Apertura de la tuberia*/
    pipe_status = pipe(fd);
    if (pipe_status == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /*Apertura la cola de mensajes del monitor*/
    monitor_queue = mq_open(MQ_MONITOR, O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
    if (monitor_queue == (mqd_t)-1)
    {
        perror("monitor queue");
        exit(EXIT_FAILURE);
    }

    if ((fd_shm = shm_open(SHM_NAME_NET, O_RDWR, S_IRUSR | S_IWUSR)) == -1)
    {
        perror("shm_open");
        mq_close(monitor_queue);
        exit(EXIT_FAILURE);
    }

    if ((struc_net = (NetData *)mmap(NULL, sizeof(NetData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(fd_shm);
        mq_close(monitor_queue);
        exit(EXIT_FAILURE);
    }
    close(fd_shm);

    /*Unión del monitor a la estructura de la memoria compartida de la red*/
    if (esperar_semaforo(&struc_net->sem_mutex) == -1)
    {
        printf("Notificación del sistema: Ha ocurrido un error-> Ejecución Detenida\n");
        exit(EXIT_FAILURE);
    }
    struc_net->monitor_pid = getpid();
    sem_post(&struc_net->sem_mutex);
    pid_main = getpid();
    printf("PID monitor: %jd\n", (intmax_t)pid_main);

    pid = fork();

    if (pid < 0)
    {
        perror("fork: ");
        liberar_recursos(blockChain, struc_net, monitor_queue);
        exit(EXIT_FAILURE);
    }

    /*****************************************************************************************Proceso Hijo*******************************************************************/
    if (pid == 0)
    {
        alarm(5);
        close(fd[1]);

        while (sig_int == 0)
        {
            prev = NULL;
            prev = (Block *)malloc(sizeof(Block));
            if (prev == NULL)
            {
                perror("malloc");
                liberar_recursos(blockChain, struc_net, monitor_queue);
                close(fd[0]);
                exit(EXIT_FAILURE);
            }
            /*Lectura de lo escrito en la tuberia*/
            while ((nbytes = read(fd[0], prev, len)) == -1)
            {
                if (nbytes == -1)
                {
                    if (errno == EINTR)
                    {
                        if (sig_int == 1)
                        {
                            break;
                        }
                        sig_alarm = 0;
                        alarm(5);
                        if (blockChain != NULL)
                        {
                            pf = fopen("log.txt", "w");
                            print_blocks(blockChain, 0, pf);
                            fclose(pf);
                        }
                        continue;
                    }
                    else
                    {
                        perror("read");
                        liberar_recursos(blockChain, struc_net, monitor_queue);
                        close(fd[0]);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (nbytes > 0)
            {
                wallets = (int *)malloc(MAX_MINERS * sizeof(int));
                if (wallets == NULL)
                {
                    perror("malloc");
                    liberar_recursos(blockChain, struc_net, monitor_queue);
                    free(prev);
                    free(wallets);
                    close(fd[0]);
                    exit(EXIT_FAILURE);
                }
                for (int l = 0; l < MAX_MINERS; l++)
                {
                    wallets[l] = prev->wallets[l];
                }
                /*Añadir bloque leido a la cadena de bloques del hijo*/
                if (add_to_chain(&blockChain, wallets, prev->target, prev->solution, prev->id, 1) == -1)
                {
                    liberar_recursos(blockChain, struc_net, monitor_queue);
                    free(wallets);
                    free(prev);
                    close(fd[0]);
                    exit(EXIT_FAILURE);
                }
                free(wallets);
            }
            if (sig_alarm == 1)
            {
                sig_alarm = 0;
                alarm(5);
                if (blockChain != NULL)
                {
                    pf = fopen("log.txt", "w");
                    if (pf == NULL)
                    {
                        perror("fopen");
                        liberar_recursos(blockChain, struc_net, monitor_queue);
                        free(prev);
                        close(fd[0]);
                        exit(EXIT_FAILURE);
                    }
                    print_blocks(blockChain, 0, pf);
                    fclose(pf);
                }
            }
            free(prev);
        }
        /* Finalizamos el proceso*/
        kill(pid_main, 2);
        liberar_recursos(blockChain, struc_net, monitor_queue);
        close(fd[0]);
        exit(EXIT_SUCCESS);
    }

    /* ******************************************************************Proceso Padre************************************************************/
    else
    {
        /* Inicializacion del buffer */
        for (l = 0; l < SIZE; l++)
        {
            buffer[l].id = 0;
        }

        close(fd[0]);
        while (sig_int == 0)
        {
            /* Recepción de los mensajes enviados por parte de la red de minería*/
            if (mq_receive(monitor_queue, (char *)&bloque_recibido, sizeof(Block), NULL) == -1)
            {
                if (errno == EINTR)
                {
                    if (sig_int == 1)
                    {
                        break;
                    }
                }
                else
                {
                    perror("mq_receive");
                    liberar_recursos(blockChain, struc_net, monitor_queue);
                    close(fd[1]);
                    kill(pid, 2);
                    wait(NULL);
                    exit(EXIT_FAILURE);
                }
            }

            aux = (Block *)malloc(sizeof(Block));
            if (aux == NULL)
            {
                perror("malloc");
                liberar_recursos(blockChain, struc_net, monitor_queue);
                close(fd[1]);
                kill(pid, 2);
                wait(NULL);
                exit(EXIT_FAILURE);
            }
            aux->target = bloque_recibido.target;
            aux->solution = bloque_recibido.solution;
            aux->id = bloque_recibido.id;
            aux->prev = NULL;
            aux->next = NULL;
            aux->is_valid = 1;

            for (int l = 0; l < MAX_MINERS; l++)
            {
                aux->wallets[l] = bloque_recibido.wallets[l];
                aux->voting_pool[l] = 0;
            }

            flag = 0;
            for (l = 0; l < SIZE_BUFFER; l++)
            {
                if (buffer[l].id == aux->id)
                {
                    flag = 1;
                    /*Comproabción por parte del monitor la veracidad del bloque recibido*/
                    if (aux->solution == buffer[l].solution && aux->target == buffer[l].target)
                        printf("Verified Block with ID: %d , target: %ld, solution: %ld\n", aux->id, aux->target, aux->solution);
                    else
                        printf("Error in Block with ID: %d , target: %ld, solution: %ld\n", aux->id, aux->target, aux->solution);
                }
            }
            if (flag == 0)
            {
                buffer[prim].id = aux->id;
                buffer[prim].target = aux->target;
                buffer[prim].solution = aux->solution;
                prim = (prim + 1) % (SIZE_BUFFER);
                nbytes = write(fd[1], aux, sizeof(Block));
                if (nbytes == -1)
                {
                    perror("write");
                    free(aux);
                    liberar_recursos(blockChain, struc_net, monitor_queue);
                    close(fd[1]);
                    kill(pid, 2);
                    wait(NULL);
                    exit(EXIT_FAILURE);
                }
            }
            free(aux);
        }
        /*Cortar el vínculo con la red de minería*/
        if (esperar_semaforo(&struc_net->sem_mutex) == -1)
        {
            printf("Notificación del sistema: Ha ocurrido un error->Ejecución detenida");
            liberar_recursos(blockChain, struc_net, monitor_queue);
            kill(pid, 2);
            wait(NULL);
            exit(EXIT_FAILURE);
        }
        struc_net->monitor_pid = -1;
        sem_post(&struc_net->sem_mutex);

        /*Liberar recursos y salir*/
        liberar_recursos(blockChain, struc_net, monitor_queue);
        kill(pid, 2);
        wait(NULL);
        exit(EXIT_SUCCESS);
    }
}
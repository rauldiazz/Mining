/**
 * @file funciones.c
 * @author Raul Diaz Bonete (raul.diazb@estudiante.uam.es)
 * @author Ignacio Bernardez Toral (ignacio.bernardez@estudiante.uam.es)
 * @brief Archivo auxiliar funciones.c, que contiene la implementación de varias funciones declaradas en miner.h que se utilizarán en miner.c.
 * @version 0.1
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "miner.h"

#define PRIME 99997669
#define BIG_X 435679812
#define BIG_Y 100001819

/**
 * @brief Funcion que inicializa la red de mineros en una estructura llamada NetData usando un mapeo de memoria compartida.
 * 
 * @param num_minero numero de minero en orden de llegada.
 * @return NetData* puntero a la estructura de la red.
 */
NetData *attachToNet(int *num_minero)
{
    int fd_shm, first = 0, l, ret = 0, f = 0;
    NetData *struc_net;

    if ((fd_shm = shm_open(SHM_NAME_NET, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
    {
        if (errno == EEXIST)
        {
            if ((fd_shm = shm_open(SHM_NAME_NET, O_RDWR, S_IRUSR | S_IWUSR)) == -1)
            {
                perror("shm_open");
            }
        }
        else
        {
            perror("shm_open");
            return NULL;
        }
    }

    else
    {

        first = 1;

        /* Resize of the memory segment. */

        if (ftruncate(fd_shm, sizeof(NetData)) == -1)
        {
            perror("ftruncate");
            close(fd_shm);
            shm_unlink(SHM_NAME_NET);
            return NULL;
        }

        /*Mapeado del segmento de memoria e inicialización del semáforo para el primer minero*/
        if ((struc_net = (NetData *)mmap(NULL, sizeof(NetData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        {
            perror("mmap");
            return NULL;
        }

        if (sem_init(&(struc_net->sem_mutex), 1, 1) == -1)
        {
            perror("sem_init");
            return NULL;
        }
        close(fd_shm);
        fd_shm = -1;

        /*Inicialización de los campos de la estructura, pues este es el primer minero*/
        ret = esperar_semaforo(&struc_net->sem_mutex);
        if (ret == -1)
        {
            return NULL;
        }
        struc_net->last_miner = 0;
        struc_net->last_winner = -1;
        struc_net->total_miners = 0;
        struc_net->monitor_pid = -1;
        struc_net->last_winner = getpid();
        for (l = 0; l < MAX_MINERS; l++)
        {
            struc_net->miners_join[l] = 0;
            struc_net->miners_pid[l] = -1;
        }
        sem_post(&struc_net->sem_mutex);

        /* Inicializacion de los semaforos por parte del primer minero */
        if (sem_init(&(struc_net->sem_block), 1, 1) == -1)
        {
            perror("sem_init");
            sem_destroy(&struc_net->sem_mutex);
            return NULL;
        }
        if (sem_init(&(struc_net->winner), 1, 1) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            perror("sem_init");
            return NULL;
        }

        if (sem_init(&(struc_net->bloque_escrito), 1, 0) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            perror("sem_init");
            return NULL;
        }
        if (sem_init(&(struc_net->bloque_recibido), 1, 0) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            sem_destroy(&struc_net->bloque_escrito);
            perror("sem_init");
            return NULL;
        }

        if (sem_init(&(struc_net->contar_votos), 1, 0) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            sem_destroy(&struc_net->bloque_escrito);
            sem_destroy(&struc_net->bloque_recibido);
            perror("sem_init");
            return NULL;
        }

        if (sem_init(&(struc_net->iniciar_votacion), 1, 0) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            sem_destroy(&struc_net->bloque_escrito);
            sem_destroy(&struc_net->bloque_recibido);
            sem_destroy(&struc_net->contar_votos);
            perror("sem_init");
            return NULL;
        }
        if (sem_init(&struc_net->cont, 1, 1) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            sem_destroy(&struc_net->bloque_escrito);
            sem_destroy(&struc_net->bloque_recibido);
            sem_destroy(&struc_net->contar_votos);
            sem_destroy(&struc_net->iniciar_votacion);
            perror("sem_init");
            return NULL;
        }

        if (sem_init(&struc_net->aux, 1, 0) == -1)
        {
            sem_destroy(&struc_net->sem_mutex);
            sem_destroy(&struc_net->sem_block);
            sem_destroy(&struc_net->winner);
            sem_destroy(&struc_net->bloque_escrito);
            sem_destroy(&struc_net->bloque_recibido);
            sem_destroy(&struc_net->contar_votos);
            sem_destroy(&struc_net->iniciar_votacion);
            sem_destroy(&struc_net->cont);
            perror("sem_init");
            return NULL;
        }
    }

    /*Mapeado del segmento de memoria e inicialización del semáforo*/
    if (first == 0)
    {
        if ((struc_net = (NetData *)mmap(NULL, sizeof(NetData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        {
            perror("mmap");
            return NULL;
        }

        close(fd_shm);
        fd_shm = -1;
    }

    /*Attach del minero a la red*/
    ret = esperar_semaforo(&struc_net->sem_mutex);
    if (ret == -1)
    {
        return NULL;
    }
    f = 0;
    for (l = 0; l < MAX_MINERS; l++)
    {
        if (struc_net->miners_pid[l] == -1)
        {
            (*num_minero) = l;
            struc_net->last_miner = (*num_minero);
            struc_net->miners_pid[*num_minero] = getpid();
            struc_net->total_miners++;
            f = 1;
            break;
        }
    }

    sem_post(&struc_net->sem_mutex);
    if (f == 0)
    {
        return NULL;
    }

    return struc_net;
}

/**
 * @brief Funcion que inicializa los bloques de un minero mediante memoria compartida.
 * 
 * @param struc_net puntero a la estructura de la red.
 * @param mq_monitor puntero a la cola de mensajes del monitor.
 * @return Block* puntero a la estructura de bloque creada.
 */
Block *initializeBlocks(NetData *struc_net, mqd_t *mq_monitor)
{
    int fd_shm_block, first = 0, l, ret = 0;
    Block *struc_block;
    struct mq_attr attributes =
        {
            .mq_flags = 0,
            .mq_maxmsg = 10,
            .mq_msgsize = sizeof(Block),
            .mq_curmsgs = 0};
    /*Apertura/Creación del segmento de memoria compartido*/
    if ((fd_shm_block = shm_open(SHM_NAME_BLOCK, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
    {
        if (errno == EEXIST)
        {
            if ((fd_shm_block = shm_open(SHM_NAME_BLOCK, O_RDWR, S_IRUSR | S_IWUSR)) == -1)
            {
                perror("shm_open");
                return NULL;
            }
        }
        else
        {
            perror("shm_open,block");
            return NULL;
        }
    }
    else //Este es el primer minero//
    {

        if (ftruncate(fd_shm_block, sizeof(Block)) == -1 && first == 1)
        {
            perror("ftruncate");
            close(fd_shm_block);
            shm_unlink(SHM_NAME_BLOCK);
            return NULL;
        }
        /*Apertura de la cola de comunicación con el monitor*/
        (*mq_monitor) = mq_open(MQ_MONITOR, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
        if ((*mq_monitor) == (mqd_t)-1)
        {
            perror("mq_open");
            return NULL;
        }

        /*Mapeado del segmento compartido del bloque*/
        if ((struc_block = (Block *)mmap(NULL, sizeof(Block), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm_block, 0)) == MAP_FAILED)
        {
            perror("mmap");
            return NULL;
        }

        ret = esperar_semaforo(&struc_net->sem_block);
        if (ret == -1)
        {
            return NULL;
        }
        struc_block->id = 0;
        struc_block->is_valid = 0;
        struc_block->next = NULL;
        struc_block->prev = NULL;
        struc_block->solution = .1;

        srand(getpid());
        struc_block->target = rand() % PRIME;
        for (l = 0; l < MAX_MINERS; l++)
        {
            struc_block->wallets[l] = 0;
        }
        sem_post(&struc_net->sem_block);

        first = 1;
    }
    /*Solo los mineros que no son los que inician la red abren la cola*/
    if (first == 0)
    {
        (*mq_monitor) = mq_open(MQ_MONITOR, O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
        if ((*mq_monitor) == (mqd_t)-1)
        {
            perror("mq_open");
            return NULL;
        }

        /*Mapeado del segmento compartido del bloque*/
        if ((struc_block = (Block *)mmap(NULL, sizeof(Block), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm_block, 0)) == MAP_FAILED)
        {
            perror("mmap");
            return NULL;
        }
    }

    close(fd_shm_block);
    fd_shm_block = -1;
    return struc_block;
}

/**
 * @brief Funcion auxiliar que reserva memoria para un array de hilos.
 * 
 * @param num_workers numero de trabajadores(hilos) para los que se reserva memoria.
 * @return pthread_t* array de hilos.
 */
pthread_t *mallocHilos(int num_workers)
{
    pthread_t *hilos;

    /*Reserva de memoria para los hilos del minero*/
    hilos = (pthread_t *)malloc(num_workers * sizeof(pthread_t));
    if (hilos == NULL)
    {
        perror("malloc");
    }
    return hilos;
}

/**
 * @brief Funcion auxiliar que reserva memoria para un array de variables de tipo worker_struct.
 * 
 * @param num_workers numero de trabajadores, posiciones para las que se reserva memoria del array.
 * @return worker_struct* array de estructuras de tipo worker_struct.
 */
worker_struct *mallocWorker(int num_workers)
{
    worker_struct *strucs;

    /*Reserva de memoria para las estructuras de cada trabajador del minero*/
    strucs = (worker_struct *)malloc(num_workers * sizeof(worker_struct));
    if (strucs == NULL)
    {
        perror("malloc");
    }
    return strucs;
}

/**
 * @brief Funcion proporcionada que calcula una determinada funcion hash.
 * 
 * @param number numero al que se le aplica la funcion hash.
 * @return long int resultado de aplicar la funcion hash.
 */
long int simple_hash(long int number)
{
    long int result = (number * BIG_X + BIG_Y) % PRIME;
    return result;
}

/**
 * @brief Funcion que imprime los bloques en un fichero de texto al final del minero.
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
 * @brief Funcion auxiliar que actualiza la estructura timespec a 10 segundos.
 * 
 * @param ts puntero a la estructura timespec a actualizar.
 */
void actualizarTiempo(struct timespec *ts)
{
    if (clock_gettime(CLOCK_REALTIME, ts) == -1)
    {
        perror("clock_gettime: ");
    }
    ts->tv_sec += 10;
}

/**
 * @brief Funcion que envia la señal SIGUSR2 a todos los mineros activos de la red.
 * 
 * @param struc_net puntero a la estructura de la red.
 */
void send_sigusr2(NetData *struc_net)
{
    int l;
    for (l = 0; l < MAX_MINERS; l++)
    {
        if (struc_net->miners_pid[l] != getpid() && struc_net->miners_pid[l] != -1 && struc_net->miners_join[l] == 1)
        {
            kill(struc_net->miners_pid[l], SIGUSR2);
        }
    }
}

/**
 * @brief Funcion que inicializa y enlaza los hilos(trabajadores) de un minero.
 * 
 * @param hilos array de hilos
 * @param struc_net puntero a la estructura de la red.
 * @param strucs array de estructuras worker_struct
 * @param struc_block cadena de bloques
 * @param num_workers numero de trabajadores(hilos)
 * @return long int solucion en caso de que se ejecute correctamente, -2 si hay error.
 */
long int inicializarHilos(pthread_t *hilos, NetData *struc_net, worker_struct *strucs, Block *struc_block, int num_workers)
{
    int l, error, ret = 0;
    long int each = PRIME / num_workers, counter = 0;
    long int solution;

    for (l = 0; l < num_workers; l++)
    {

        ret = esperar_semaforo(&struc_net->sem_block);
        if (ret == -1)
        {
            return ret;
        }
        strucs[l].target = struc_block->target;
        sem_post(&struc_net->sem_block);

        strucs[l].solution = -1;

        (strucs[l]).start = counter;

        counter += each;
        if (l != num_workers - 1)
        {
            (strucs[l]).end = counter;
        }
        else
        {
            (strucs[l]).end = PRIME;
        }

        error = pthread_create(&(hilos[l]), NULL, Busqueda, (void *)&(strucs[l]));
        if (error != 0)
        {
            perror("pthread_create");
            return -2;
        }
    }

    for (l = 0; l < num_workers; l++)
    {

        error = pthread_join((hilos[l]), NULL);
        if (error != 0)
        {
            perror("pthread_join");
            return -2;
        }
        if ((strucs[l].solution) != -1)
        {
            solution = strucs[l].solution;
        }
    }
    return solution;
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
 * @brief Funcion que escribe la solucion en el bloque y manda SIGUSR2 al resto de mineros.
 * 
 * @param struc_net puntero a la estructura de la red.
 * @param struc_block estructura del bloque.
 * @param solution solucion a la ronda de minado.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int escritura_solucion(NetData *struc_net, Block *struc_block, long int solution)
{
    int ret = 0;
    /*Escritura de la solución*/
    ret = esperar_semaforo(&struc_net->sem_block); //Protección Sem_Block
    if (ret == -1)
    {
        return ret;
    }
    struc_block->solution = solution;
    sem_post(&struc_net->sem_block); //Fin def protección Sem_Block

    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    struc_net->last_winner = getpid();
    printf("Last winner: %jd\n", (intmax_t)struc_net->last_winner);
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData

    send_sigusr2(struc_net);
    return 1;
}

/**
 * @brief Funcion para empezar el proceso de votacion de un bloque, poniendo todos los votos inicialmente a 0.
 * 
 * @param struc_net puntero de estructura a la red.
 * @param struc_block puntero a la estructura del bloque.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int iniciar_votacion(NetData *struc_net, Block *struc_block)
{
    int l, ret;
    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion Block
    if (ret == -1)
    {
        return ret;
    }
    for (l = 0; l < MAX_MINERS; l++)
    {
        struc_block->voting_pool[l] = 0;
    }
    sem_post(&struc_net->sem_block); // Fin de proteccion Block

    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    for (l = 1; l < struc_net->active_miners; l++)
    {
        sem_post(&struc_net->iniciar_votacion);
    }
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData
    return 1;
}

/**
 * @brief Funcion que realiza la espera adecuada hasta que se haya completado la votacion del bloque.
 * 
 * @param struc_net puntero de estructura a la red.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int esperar_votacion(NetData *struc_net)
{
    int l, ret = 0;
    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    for (l = 1; l < struc_net->active_miners; l++)
    {
        esperar_semaforo(&struc_net->contar_votos);
    }
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData
    return 1;
}

/**
 * @brief Funcion que verifica si la votacion de un bloque ha sido aceptada o denegada.
 * 
 * @param struc_net puntero de estructura a la red.
 * @param struc_block bloque que se ha votado.
 * @param num_minero numero de minero que ha iniciado la votacion.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int consultar_votacion(NetData *struc_net, Block *struc_block, int num_minero)
{
    int contvotos, resultado, l, ret = 0;

    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion de Block
    if (ret == -1)
    {
        return ret;
    }
    contvotos = 0;
    for (l = 0; l < MAX_MINERS; l++)
    {
        contvotos += struc_block->voting_pool[l];
    }

    sem_post(&struc_net->sem_block); //Fin de proteccion de Block

    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    resultado = (contvotos + 1.0) / (struc_net->active_miners * 1.0);

    struc_net->total_miners = struc_net->total_miners - struc_net->salidas;
    struc_net->salidas = 0;
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData

    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion de Block
    if (ret == -1)
    {
        return ret;
    }
    struc_block->is_valid = 0;
    if (resultado > 0.5)
    {
        struc_block->is_valid = 1;
        struc_block->wallets[num_minero] += 1;
    }

    sem_post(&struc_net->sem_block); //Fin de proteccion de Block
    return 1;
}

/**
 * @brief Funcion que actualiza la cadena de bloques y establece el target de la siguiente ronda.
 * 
 * @param struc_net puntero de estructura a la red.
 * @param struc_block puntero de estructura del bloque.
 * @param blockChain cadena de bloques.
 * @param mq_monitor cola de mensajes con el monitor.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int actualizacion_bloque(NetData *struc_net, Block *struc_block, Block **blockChain, mqd_t mq_monitor)
{
    int *wallets;
    int l, ret = 0;
    Block new;

    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    for (l = 1; l < struc_net->active_miners; l++)
    {
        sem_post(&struc_net->bloque_escrito);
    }
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData

    /*Espera a la recepción del bloque*/
    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    for (l = 1; l < struc_net->active_miners; l++)
    {
        ret = esperar_semaforo(&struc_net->bloque_recibido);
        if (ret == -1)
        {
            return ret;
        }
    }
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData

    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion de Block
    if (ret == -1)
    {
        return ret;
    }

    if (struc_block->is_valid == 1)
    {

        wallets = (int *)malloc(MAX_MINERS * sizeof(int));
        if (wallets == NULL)
        {
            perror("malloc");
            return -1;
        }
        for (int j = 0; j < MAX_MINERS; j++)
        {
            new.wallets[j] = struc_block->wallets[j];
            wallets[j] = struc_block->wallets[j];
        }

        if (add_to_chain(blockChain, wallets, struc_block->target, struc_block->solution, struc_block->id, struc_block->is_valid) == -1)
        {
            return -1;
        }

        new.target = struc_block->target;
        new.solution = struc_block->solution;
        new.id = struc_block->id;
        new.is_valid = 1;
        new.next = NULL;
        new.prev = NULL;

        struc_block->target = struc_block->solution;
        struc_block->id++;
        free(wallets);
    }
    else
    {
        wallets = (int *)malloc(MAX_MINERS * sizeof(int));
        if (wallets == NULL)
        {
            perror("malloc");
            return -1;
        }
        for (int j = 0; j < MAX_MINERS; j++)
        {
            new.wallets[j] = struc_block->wallets[j];
            wallets[j] = struc_block->wallets[j];
        }

        new.target = struc_block->target;
        new.solution = struc_block->solution;
        new.id = struc_block->id;
        new.is_valid = 1;
        new.next = NULL;
        new.prev = NULL;
        free(wallets);
    }

    sem_post(&struc_net->sem_block); //Fin de proteccion de Block

    ret = esperar_semaforo(&struc_net->sem_mutex);
    if (ret == -1)
    {
        return ret;
    }
    if (struc_net->monitor_pid != -1)
    {
        mq_send(mq_monitor, (const char *)&new, sizeof(Block), 2);
    }
    struc_net->active_miners = 0;
    sem_post(&struc_net->sem_mutex);
    return 1;
}

/**
 * @brief Funcion que realiza la votacion del minero para cierto bloque.
 * 
 * @param struc_net puntero de estructura a la red.
 * @param struc_block bloque que se va a votar.
 * @param num_minero numero de minero que vota.
 * @return int 1 si se ejecuta correctamente, -1 si hay error.
 */
int realizar_votacion(NetData *struc_net, Block *struc_block, int num_minero)
{
    int ret = 0;
    ret = esperar_semaforo(&struc_net->iniciar_votacion);
    if (ret == -1)
    {
        return ret;
    }

    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion Sem_block
    if (ret == -1)
    {
        return ret;
    }
    if (simple_hash(struc_block->solution) == struc_block->target)
    {
        struc_block->voting_pool[num_minero] = 1;
    }

    sem_post(&struc_net->sem_block);

    sem_post(&struc_net->contar_votos);
    ret = esperar_semaforo(&struc_net->bloque_escrito);
    if (ret == -1)
    {
        return ret;
    }
    return 1;
}

/**
 * @brief Funcion que actualiza la cadena de bloques y el bloque indicado. 
 * 
 * @param struc_net puntero a la estructura de la red.
 * @param struc_block bloque del que se ha encontrado la solucion (aceptada o denegada).
 * @param blockChain cadena de bloques.
 * @param mq_monitor cola de mensajes con el monitor.
 * @return int 1 si se ha ejecutado correctamente, -1 si hay error.
 */
int actualizar_cadena(NetData *struc_net, Block *struc_block, Block **blockChain, mqd_t mq_monitor)
{
    int ret;
    int *wallets;
    Block new;

    ret = esperar_semaforo(&struc_net->sem_block); //Proteccion Sem_block
    if (ret == -1)
    {
        return ret;
    }
    if (struc_block->is_valid == 1)
    {

        wallets = (int *)malloc(MAX_MINERS * sizeof(int));
        if (wallets == NULL)
        {
            perror("malloc");
            return -1;
        }
        for (int j = 0; j < MAX_MINERS; j++)
        {
            new.wallets[j] = struc_block->wallets[j];
            wallets[j] = struc_block->wallets[j];
        }

        new.target = struc_block->target;
        new.solution = struc_block->solution;
        new.id = struc_block->id;
        new.is_valid = 1;
        new.next = NULL;
        new.prev = NULL;
        ret = add_to_chain(blockChain, wallets, struc_block->target, struc_block->solution, struc_block->id, struc_block->is_valid);
        if (ret == -1)
        {
            return ret;
        }
        free(wallets);
    }
    else
    {

        wallets = (int *)malloc(MAX_MINERS * sizeof(int));
        if (wallets == NULL)
        {
            perror("malloc");
            return -1;
        }
        for (int j = 0; j < MAX_MINERS; j++)
        {
            new.wallets[j] = struc_block->wallets[j];
            wallets[j] = struc_block->wallets[j];
        }

        new.target = struc_block->target;
        new.solution = struc_block->solution;
        new.id = struc_block->id;
        new.is_valid = 1;
        new.next = NULL;
        new.prev = NULL;
        free(wallets);
    }

    sem_post(&struc_net->sem_block); //Fin de proteccion Sem_block

    sem_post(&struc_net->bloque_recibido);
    ret = esperar_semaforo(&struc_net->sem_mutex); //Proteccion NetData
    if (ret == -1)
    {
        return ret;
    }
    if (struc_net->monitor_pid != -1)
    {
        mq_send(mq_monitor, (const char *)&new, sizeof(Block), 1);
    }
    sem_post(&struc_net->sem_mutex); //Fin de proteccion NetData
    return 1;
}

/**
 * @brief Funcion que crea un nuevo bloque con los parametros indicados.
 * 
 * @param target target del bloque.
 * @param solution solucion del bloque.
 * @param id id del bloque
 * @return Block* puntero al bloque creado.
 */
Block *BlockCreate(long int target, long int solution, int id)
{
    Block *block;
    int j;

    block = (Block *)malloc(sizeof(Block));
    if (block == NULL)
    {
        perror("malloc");
        return NULL;
    }

    for (j = 0; j < MAX_MINERS; j++)
    {
        block->wallets[j] = 0;
    }
    block->target = target;
    block->solution = solution;
    block->id = id;
    block->is_valid = 0;
    block->next = NULL;
    block->prev = NULL;
    return block;
}

/**
 * @brief Funcion que establece los manejadores de la señal SIGINT, SIGUSR2 y SIGALARM del minero.
 * 
 * @return int 0 si se ejecuta correctamente, -1 si hay error.
 */
int establecer_manejadores()
{

    struct sigaction act_sigint, act_sigusr2, act_sigalarm;
    /*Declaración del manejador de la señal SIGINT*/
    act_sigint.sa_handler = manejadorSigInt;
    sigfillset(&(act_sigint.sa_mask));
    act_sigint.sa_flags = 0;

    if (sigaction(SIGINT, &act_sigint, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    /*Declaración del manejador de la señal SIGUSR2*/
    act_sigusr2.sa_handler = manejadorSigUsr2;
    sigfillset(&(act_sigusr2.sa_mask));
    act_sigusr2.sa_flags = 0;

    if (sigaction(SIGUSR2, &act_sigusr2, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    act_sigalarm.sa_handler = manejadorSigAlarm;
    sigfillset(&(act_sigalarm.sa_mask));
    act_sigalarm.sa_flags = 0;

    if (sigaction(SIGALRM, &act_sigalarm, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }
    return 0;
}

/**
 * @brief Funcion que libera los recursos del minero y escribe en un fichero su cadena de bloques.
 * 
 * @param hilos array de hilos.
 * @param struc_net puntero a estructura de la red.
 * @param struc_block puntero a estructura de bloque.
 * @param strucs array de worker_struct
 * @param mq_monitor cola de mensajes con el monitor.
 * @param num_minero numero de minero en la red.
 * @param blockChain cadena de bloques del minero.
 */
void liberar_recursos(pthread_t *hilos, NetData *struc_net, Block *struc_block, worker_struct *strucs, mqd_t mq_monitor, int num_minero, Block *blockChain)
{
    char buffer[MAX_MINERS];
    Block *aux2, *aux3;
    FILE *f;

    esperar_semaforo(&struc_net->sem_mutex);

    struc_net->miners_pid[num_minero] = -1;
    struc_net->miners_join[num_minero] = 0;

    if (struc_net->total_miners == 0)
    {

        shm_unlink(SHM_NAME_BLOCK);
        shm_unlink(SHM_NAME_NET);
        mq_unlink(MQ_MONITOR);
        snprintf(buffer, 200, "%jd\n", (intmax_t)getpid());

        f = fopen(buffer, "w");
        print_blocks(blockChain, 3, f);
        fclose(f);
        //sem_post(&struc_net->sem_mutex);
        sem_destroy(&struc_net->aux);
        sem_destroy(&struc_net->cont);
        sem_destroy(&struc_net->bloque_escrito);
        sem_destroy(&struc_net->bloque_recibido);
        sem_destroy(&struc_net->iniciar_votacion);
        sem_destroy(&struc_net->contar_votos);
        sem_destroy(&struc_net->winner);
        sem_destroy(&struc_net->sem_mutex);
        sem_destroy(&struc_net->sem_block);
        munmap(struc_block, sizeof(Block));
        munmap(struc_net, sizeof(Block));
        mq_close(mq_monitor);

        if (blockChain != NULL)
        {
            aux2 = blockChain;
            while (aux2 != NULL)
            {
                aux3 = aux2;
                aux2 = aux2->prev;
                free(aux3);
            }
        }
        if (hilos != NULL)
        {
            free(hilos);
        }
        if (strucs != NULL)
        {
            free(strucs);
        }

        return;
    }
    sem_post(&struc_net->sem_mutex);

    snprintf(buffer, 200, "%jd\n", (intmax_t)getpid());
    f = fopen(buffer, "w");
    print_blocks(blockChain, 3, f);
    fclose(f);

    if (hilos != NULL)
    {
        free(hilos);
    }
    if (strucs != NULL)
    {
        free(strucs);
    }

    return;
}

/**
 * @brief Funcion que da inicio a una nueva ronda de minado.
 * 
 * @param flag_salida 
 * @param struc_net 
 * @param num_minero 
 * @param sig_int 
 * @param num_rounds 
 * @param m 
 * @return int 
 */
int inicio_ronda(int *flag_salida, NetData *struc_net, int num_minero, volatile int sig_int, int num_rounds, int m)
{
    int l;
    printf("Esperando que empiece ronda %d\n", m);
    if (esperar_semaforo(&struc_net->cont) == -1)
    {
        return -1;
    }
    if (esperar_semaforo(&struc_net->sem_mutex) == -1)
    {
        return -1;
    }
    if (m + 1 == num_rounds)
    {
        struc_net->salidas += 1;
    }
    if (sig_int == 1)
    {
        (*flag_salida) = 1;
        struc_net->salidas += 1;
    }
    struc_net->active_miners++;
    struc_net->miners_join[num_minero] = 1;
    if (struc_net->total_miners == struc_net->active_miners)
    {
        for (l = 0; l < struc_net->active_miners; l++)
        {

            sem_post(&struc_net->aux);
        }
        sem_post(&struc_net->sem_mutex);
    }
    else
    {
        sem_post(&struc_net->sem_mutex);
        sem_post(&struc_net->cont);
    }

    esperar_semaforo(&struc_net->aux);
    return 1;
}
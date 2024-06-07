#include <stdlib.h>
#include <stdio.h>
// #include <utils/hello.h>
#include <utils/utils.h>
#include <utils/utils.c>
#include <commons/log.h>
#include <commons/config.h>
#include <semaphore.h>
#include "string.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <utils/pcb.h>
#include <utils/pcb.c>

char *config;
int socket_servidor_kernel;
int conexion_cpu_dispatch;
char *ip_memoria;
char *puerto_memoria;
char *mensaje;
char *modulo_cpu;
char *ip_cpu;
char *puerto_cpu_interrupt;
char *puerto_cpu_dispatch;
char *modulo_memoria;
pthread_t hilo_entradasalida;
t_log *logger;

// contadores
int contador_procesos = 0;
uint32_t pid = 0;

// listas
t_list *procesos_new;
t_list *procesos_ready;

// semaforos
sem_t sem_entradasalida;
sem_t semaforo_multiprogramacion;
sem_t cantidad_procesos_new;
sem_t proceso_ejecutando;
sem_t cantidad_procesos_ready;
uint32_t grado_maximo_multiprogramacion = 2;

// funciones
void planificador_largo_plazo();
void planificador_corto_plazo();
void FIFO();
void consola3();
void enviar_operacion_io();

// hilos
pthread_t planificadorLargoPlazo;
pthread_t ejecutarProceso;
pthread_t planificadorCortoPlazo;
pthread_t ejecutarConsola;
t_pcb *proceso_en_ejecucion;

// sockets
int conexion_memoria;

// Estructura para almacenar la info de la interfaz
typedef struct
{
    char *nombre;
    char *tipo;
} t_interfaz;

t_interfaz *interfaz_info;

char **_string_split(char *text, char *separator, bool (*is_last_token)(int));

void *conexion_entradasalida(void *arg)
{

    // t_log *logger = (t_log *)arg;

    log_info(logger, "[conexion_entradasalida] Hilo de conexión de entrada/salida iniciado.");
    int *fd_conexion_ptr = (int *)arg;  // Castear el argumento a (int *)
    int fd_conexion = *fd_conexion_ptr; // Desreferenciar el puntero para obtener el descriptor del socket

    // Recibir el nombre de la interfaz
    uint32_t nombre_length;
    recv(fd_conexion, &nombre_length, sizeof(uint32_t), 0);
    char *nombre_interfaz = malloc(nombre_length);
    recv(fd_conexion, nombre_interfaz, nombre_length, 0);

    // Recibir el tipo de interfaz
    uint32_t tipo_length;
    recv(fd_conexion, &tipo_length, sizeof(uint32_t), 0);
    char *tipo_interfaz = malloc(tipo_length);
    recv(fd_conexion, tipo_interfaz, tipo_length, 0);

    // Almacenar la información de la interfaz
    interfaz_info = malloc(sizeof(t_interfaz));
    interfaz_info->nombre = nombre_interfaz;
    interfaz_info->tipo = tipo_interfaz;

    log_info(logger, "Interfaz recibida: Nombre = %s, Tipo = %s", nombre_interfaz, tipo_interfaz);

    enviar_operacion_io(logger, fd_conexion, "IO_GEN_SLEEP");

    while (1)
    {
        int cod_op_entradasalida = recibir_operacion(fd_conexion);
        if (cod_op_entradasalida == -1)
        {
            log_error(logger, "Error al recibir operación. Reintentando...");
            continue;
        }

        recibir_mensaje(logger, fd_conexion);

        log_info(logger, "Hilo Entrada/Salida de Kernel recibió el mensaje");

        // sem_post(&sem_entradasalida); // Libera el semáforo para permitir que se cree un nuevo hilo
    }

    return NULL;
}

void enviar_operacion_io(t_log *logger, int conexion_entradasalida, const char *instruccion)
{
    uint32_t mensaje_length = strlen(instruccion) + 1; // Incluir el carácter nulo

    // Enviar el tamaño del mensaje
    if (send(conexion_entradasalida, &mensaje_length, sizeof(uint32_t), 0) == -1)
    {
        log_error(logger, "Error enviando el tamaño del mensaje.");
        return;
    }

    // Enviar el mensaje
    if (send(conexion_entradasalida, instruccion, mensaje_length, 0) == -1)
    {
        log_error(logger, "Error enviando el mensaje.");
        return;
    }

    log_info(logger, "Mensaje '%s' enviado correctamente a Entrada/Salida.", instruccion);
}

int main(int argc, char *argv[])
{

    logger = log_create("kernel.log", "kernel_main", 1, LOG_LEVEL_INFO);

    procesos_new = list_create();
    procesos_ready = list_create();
    sem_init(&semaforo_multiprogramacion, 0, grado_maximo_multiprogramacion);
    sem_init(&cantidad_procesos_new, 0, 0);
    sem_init(&cantidad_procesos_ready, 0, 0);
    sem_init(&proceso_ejecutando, 0, 0);

    sem_init(&sem_entradasalida, 0, 1);

    // consola3();

    t_config *config;

    if (logger == NULL)
    {
        perror("Error en el logger");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Kernel funcionando OK, configuracion en lectura OK.");

    config = NULL;
    config = config_create("/home/utnso/Desktop/tp-2024-1c-GFALT/kernel/kernel.config");

    if (config == NULL)
    {
        perror("Error al crear la configuración.");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Configuración leida OK.");

    char *puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    log_info(logger, "El puerto de escucha es: %s", puerto_escucha);

    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    modulo_memoria = config_get_string_value(config, "MODULO_MEMORIA");
    modulo_cpu = config_get_string_value(config, "MODULO_CPU");
    puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    ip_cpu = config_get_string_value(config, "IP_CPU");
    mensaje = config_get_string_value(config, "MENSAJE");

    log_info(logger, "ips y puertos de memoria y cpu leido correctamente");

    conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria, modulo_memoria);

    enviar_mensaje(mensaje, conexion_memoria);
    log_info(logger, "Conexion con memoria realizada correctamente");

    conexion_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch, modulo_cpu);
    // Hacer en un hilo
    enviar_mensaje(mensaje, conexion_cpu_dispatch);
    log_info(logger, "Conexion con cpu_dispatch realizada correctamente");

    int conexion_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt, modulo_cpu);
    // Hacer en un hilo
    enviar_mensaje(mensaje, conexion_cpu_interrupt);
    log_info(logger, "Conexion con cpu_interrupt realizada correctamente");

    socket_servidor_kernel = iniciar_servidor(logger, NULL, puerto_escucha);
    log_info(logger, "Servidor listo para recibir a interfaz de entrada/salida");

    pthread_create(&ejecutarConsola, NULL, (void *)consola3, NULL);
    pthread_create(&planificadorLargoPlazo, NULL, (void *)planificador_largo_plazo, NULL);
    pthread_create(&planificadorCortoPlazo, NULL, (void *)planificador_corto_plazo, NULL);
    pthread_create(&ejecutarProceso, NULL, (void *)FIFO, NULL);

    while (1)
    {
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = accept(socket_servidor_kernel, NULL, NULL);
        pthread_create(&thread, NULL, conexion_entradasalida, fd_conexion_ptr);
        pthread_detach(thread);
    }

    pthread_join(ejecutarConsola, NULL);
    pthread_join(planificadorLargoPlazo, NULL);
    pthread_join(planificadorCortoPlazo, NULL);
    pthread_join(ejecutarProceso, NULL);

    return 0;
}

void iniciar_proceso(char *path)
{
    if (path != NULL)
    {
        // Lógica para iniciar un proceso con el archivo en la ruta 'path'
        log_info(logger, "Iniciando proceso con path: %s\n", path);
        t_pcb *pcb = crear_pcb(pid);
        pid++;
        pcb->path = path;
        list_add(procesos_new, pcb);
        sem_post(&cantidad_procesos_new);
        log_info(logger, "PCB con pid: %d, pc: %d, path: %s\n", pcb->pid, pcb->pc, pcb->path);
    }
    else
    {
        log_info(logger, "Error: Faltan argumentos para INICIAR_PROCESO\n");
    }

    return;
}

void leerScript()
{
    FILE *archivo;
    char linea[100];
    char *token;

    archivo = fopen("comandos.txt", "r");

    if (archivo == NULL)
    {
        perror("Error al abrir el archivo");
        return 1;
    }

    while (fgets(linea, sizeof(linea), archivo))
    {
        // Remover el carácter de nueva línea al final de la línea
        linea[strcspn(linea, "\n")] = '\0';

        // Separar el comando en acción y parámetros
        token = strtok(linea, " ");

        if (strcmp(token, "INICIAR_PROCESO") == 0)
        {
            token = strtok(NULL, " ");
            iniciar_proceso(token);
        }
    }

    fclose(archivo);

    return 0;
}

void consola3()
{
    char linea[100];
    char *token;

    while (1)
    {
        printf("1. EJECUTAR_SCRIPT [PATH]\n");
        printf("2. INICIAR_PROCESO [PATH]\n");
        printf("3. FINALIZAR_PROCESO [PID]\n");
        printf("4. DETENER_PLANIFICACION\n");
        printf("5. INICIAR_PLANIFICACION\n");
        printf("6. MULTIPROGRAMACION [VALOR]\n");
        printf("7. PROCESO_ESTADO\n");
        printf("Ingrese una opción: \n");

        fgets(linea, sizeof(linea), stdin);
        linea[strcspn(linea, "\n")] = '\0';

        token = strtok(linea, " ");
        if (strcmp(token, "EJECUTAR_SCRIPT") == 0)
        {
            char *path = strtok(NULL, " ");
            if (path != NULL)
            {
                // Lógica para ejecutar el script en la ruta especificada
                leerScript();
                return 0;
            }
            else
            {
                printf("Error: Faltan argumentos para EJECUTAR_SCRIPT\n");
            }
        }
        else if (strcmp(token, "INICIAR_PROCESO") == 0)
        {
            char *path = strtok(NULL, " ");
            if (path != NULL)
            {
                iniciar_proceso(path);
            }
            else
            {
                printf("Error: Faltan argumentos para INICIAR_PROCESO\n");
            }
        }
        else if (strcmp(token, "FINALIZAR_PROCESO") == 0)
        {
            char *pid = strtok(NULL, " ");
            if (pid != NULL)
            {
                // Lógica para finalizar el proceso con el ID 'pid'
                printf("Finalizando proceso con ID: %s\n", pid);
            }
            else
            {
                printf("Error: Faltan argumentos para FINALIZAR_PROCESO\n");
            }
        }
        else if (strcmp(token, "DETENER_PLANIFICACION") == 0)
        {
            // Lógica para detener la planificación
            printf("Deteniendo planificación\n");
        }
        else if (strcmp(token, "INICIAR_PLANIFICACION") == 0)
        {
            // Lógica para iniciar la planificación
            printf("Iniciando planificación\n");
        }
        else if (strcmp(token, "MULTIPROGRAMACION") == 0)
        {
            char *valor = strtok(NULL, " ");
            if (valor != NULL)
            {
                // Lógica para la multiprogramación con el valor 'valor'
                printf("Realizando multiprogramación con valor: %s\n", valor);
                int valor_semaforo;
                int nuevo_grado = atoi(valor);
                sem_getvalue(&semaforo_multiprogramacion, &valor_semaforo);
                printf("Grado multiprogramacion actual %d\n", valor_semaforo);

                // int resta = nuevo_grado - valor_semaforo;
                // printf("La resta da:%d\n",resta);
                if (nuevo_grado > valor_semaforo) // 4>2
                {
                    for (int i = 0; i < nuevo_grado - valor_semaforo; i++) // 4-2

                        sem_post(&semaforo_multiprogramacion);
                }
                else
                {
                    for (int i = 0; i < valor_semaforo - nuevo_grado; i++) // 2-1

                        sem_wait(&semaforo_multiprogramacion);
                }
                sem_getvalue(&semaforo_multiprogramacion, &valor_semaforo);
                printf("Grado multiprogramacion nuevo %d\n", valor_semaforo);
            }
            else
            {
                printf("Error: Faltan argumentos para MULTIPROGRAMACION\n");
            }
        }
        else if (strcmp(token, "PROCESO_ESTADO") == 0)
        {
            imprimir_estado_procesos();
        }
        else
        {
            printf("Opción no válida. Intente nuevamente.\n");
        }
    }
}

void planificador_largo_plazo()
{
    while (1)
    {
        sem_wait(&cantidad_procesos_new);
        log_info(logger, "Entré a la planificación de LARGO plazo\n");
        sem_wait(&semaforo_multiprogramacion);

        t_pcb *pcb = list_remove(procesos_new, 0);
        pcb->estado = 1;
        list_add(procesos_ready, pcb);
        sem_post(&cantidad_procesos_ready);
        log_info(logger, "Se agregó el proceso con pid %d a la cola de ready\n", pcb->pid);
    }
}

void planificador_corto_plazo()
{
    // Grado de multiprogramación 1
    sem_wait(&cantidad_procesos_ready);
    // Grado de multiprogramación 0
    log_info(logger, "Entré a la planificación de CORTO plazo\n");
    sem_post(&proceso_ejecutando);
    // Proceso ejecutando 1, arranca FIFO
    while (1)
    {
        sem_wait(&cantidad_procesos_ready);
        log_info(logger, "Entré a la planificación de CORTO plazo\n");
    }
}

void FIFO()
{

    while (1)
    {
        // Inicialmente está en 0, cuando tiene 1 arranca
        sem_wait(&proceso_ejecutando);
        // Proceso ejecutando = 0, pongo el semaforo de este hilo en 0
        proceso_en_ejecucion = list_remove(procesos_ready, 0);
        proceso_en_ejecucion->estado = 2;
        log_info(logger, "Ejecutando proceso con pid %d\n", proceso_en_ejecucion->pid);
        log_info(logger, "Por enviar mensaje: %s", proceso_en_ejecucion->path);
        // enviar_mensaje(proceso_en_ejecucion->path,conexion_memoria);
        enviar_path(conexion_memoria, proceso_en_ejecucion->path);
        log_info(logger, "Se envió mensaje %s", proceso_en_ejecucion->path);
        t_paquete *paquete = serializar_pcb(proceso_en_ejecucion);
        enviar_pcb(conexion_cpu_dispatch, paquete);
    }
}

void imprimir_estado_procesos()
{
    printf("Estado READY: \n");
    for (int i = 0; i < list_size(procesos_ready); i++)
    {
        t_pcb *pcb = list_get(procesos_ready, i);
        printf("Proceso con pid: %d\n", pcb->pid);
    }
    printf("------\n");

    printf("Estado NEW: \n");
    for (int i = 0; i < list_size(procesos_new); i++)
    {
        t_pcb *pcb = list_get(procesos_new, i);
        printf("Proceso con pid: %d\n", pcb->pid);
    }
    printf("------\n");

    printf("Estado EXEC: \n");
    printf("Proceso con pid: %d\n", proceso_en_ejecucion->pid);
    printf("\n");
}

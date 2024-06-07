#include <stdlib.h>
#include <stdio.h>
#include <utils/utils.h>
#include <utils/utils.c>
#include <commons/log.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <pthread.h>
#include <unistd.h>

// Variables globales
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
char *modulo_kernel;
char *mensaje;
char *modulo_memoria;
char *puerto_escucha;
char *nombre;
char *tipo_interfaz;
int tiempo_unidad_trabajo;
t_log *logger;
int socket_servidor_interfaz;

char *recibir_operacion_kernel(int fd, int *size)
{
    uint32_t mensaje_length;
    if (recv(fd, &mensaje_length, sizeof(uint32_t), 0) <= 0)
    {
        return NULL;
    }

    char *mensaje = malloc(mensaje_length);
    if (recv(fd, mensaje, mensaje_length, 0) <= 0)
    {
        free(mensaje);
        return NULL;
    }

    *size = mensaje_length;
    return mensaje;
}
// Función para validar la operación recibida
int validar_operacion(const char *operacion)
{
    if (strcmp(tipo_interfaz, "GENERICA") == 0)
    {
        return strcmp(operacion, "IO_GEN_SLEEP") == 0;
    }
    return 0;
}

// void notificar_kernel(int fd_conexion)
// {
//     enviar_mensaje("Operacion completada", fd_conexion);
//     log_info(logger, "Operacion completada notificada al Kernel");
// }

void *conectar_kernel(void *arg)
{
    int *fd_conexion_ptr = (int *)arg;
    int fd_conexion = *fd_conexion_ptr;
    free(fd_conexion_ptr);

    log_info(logger, "[conexion_kernel] Hilo de conexión de entrada/salida iniciado.");

    while (1)
    {
        int size;
        char *instruccion_kernel = recibir_operacion_kernel(fd_conexion, &size);
        if (instruccion_kernel == NULL)
        {
            log_error(logger, "Error al recibir el mensaje.");
            return NULL;
        }

        log_info(logger, "Me llegó la operacion <%s> en el socket %d", instruccion_kernel, fd_conexion);

        if (validar_operacion(instruccion_kernel))
        {
            log_info(logger, "La interfaz %s de tipo %s esperará %d unidades de tiempo", nombre, tipo_interfaz, tiempo_unidad_trabajo);
            sleep(tiempo_unidad_trabajo);
            log_info(logger, "La interfaz %s de tipo %s terminó de esperar %d unidades de tiempo", nombre, tipo_interfaz, tiempo_unidad_trabajo);
        }
        else
        {
            log_error(logger, "Operación no soportada: %s\n", instruccion_kernel);
        }

        log_info(logger, "Hilo Entrada/Salida de Kernel recibió el mensaje");
        free(instruccion_kernel);
    }

    return NULL;
}

int main(int argc, char *argv[])
{

    t_config *config;
    t_log *logger;

    nombre = readline("Inserte el nombre de la interfaz\n>");

    logger = log_create("/home/utnso/Desktop/tp-2024-1c-GFALT/entradasalida/interfaz.log", "entradasalida_main", 1, LOG_LEVEL_INFO);
    if (logger == NULL)
    {
        perror("algo paso con el logger");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "EL nombre de la interfaz es: %s\n", nombre);
    log_info(logger, "entrada/salida funcionando OK, configuración en lectura OK.");

    config = config_create("/home/utnso/Desktop/tp-2024-1c-GFALT/entradasalida/interfaz1.config");
    if (config == NULL)
    {
        perror("la configuración no fue leída correctamente.");
        exit(EXIT_FAILURE);
    }

    // Leer configuraciones del archivo
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    modulo_kernel = config_get_string_value(config, "MODULO_KERNEL");
    modulo_memoria = config_get_string_value(config, "MODULO_MEMORIA");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    mensaje = config_get_string_value(config, "MENSAJE");
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

    log_info(logger, "EL tipo de la interfaz es: %s\n", tipo_interfaz);
    log_info(logger, "IPs y puertos de entrada/salida y memoria leídos correctamente");

    // Conexión con memoria
    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria, modulo_memoria);
    enviar_mensaje(mensaje, conexion_memoria);
    log_info(logger, "Conexión con memoria realizada correctamente");

    // Conexión con kernel
    int conexion_kernel = crear_conexion(logger, ip_kernel, puerto_kernel, modulo_kernel);

    // Enviar el nombre y el tipo de interfaz al kernel
    uint32_t nombre_length = strlen(nombre) + 1;
    send(conexion_kernel, &nombre_length, sizeof(uint32_t), 0);
    send(conexion_kernel, nombre, nombre_length, 0);

    uint32_t tipo_length = strlen(tipo_interfaz) + 1;
    send(conexion_kernel, &tipo_length, sizeof(uint32_t), 0);
    send(conexion_kernel, tipo_interfaz, tipo_length, 0);
    log_info(logger, "Conexión con kernel realizada correctamente");

    // Iniciar servidor para recibir conexiones del kernel
    socket_servidor_interfaz = iniciar_servidor(logger, NULL, puerto_escucha);
    log_info(logger, "Servidor listo para recibir al kernel");

    while (1)
    {
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = accept(socket_servidor_interfaz, NULL, NULL);
        pthread_create(&thread, NULL, conectar_kernel, fd_conexion_ptr);
        pthread_detach(thread);
    }

    return 0;
}

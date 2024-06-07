#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/pcb.h>
#include <utils/pcb.c>
#include <utils/utils.h>
#include <utils/utils.c>
#define MAX_LINE_LENGTH 256

char* IP;
char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
int PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

extern char *path_instrucciones;

t_log* logger;
t_config* config;
t_dictionary *lista_instruccion_pid;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_consola(t_log*);
void crear_proceso(int socket_module_cliente);
void enviar_instruccion_a_cpu(int socket_module_cliente, int retardo_respuesta);
void leer_archivopseudocode(int socket_module_cliente);
void terminar_programa(t_log*, t_config*);
void iterator(char* value, t_log*);


#endif /* MAIN_H_ */

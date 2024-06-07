#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<signal.h>
#include<utils/pcb.h>

#define PUERTO "4444" // --DEBATIR TEMA DEL PUERTO

typedef enum{
	SET,
	MOV_IN,
	MOV_OUT,
	SUM,
	SUB,
	JNZ,
	RESIZE,
	COPY_STRING,
	WAIT,
	SIGNAL,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	EXIT
} cod_instruccion;
typedef struct{
	int instruccion_lenght;
	char* codinstruccion;
	char* parametros[3];
	int parametro1_lenght;
	int parametro2_lenght;
	int parametro3_lenght;
} t_instruccion;
typedef enum
{
	MENSAJE,
	INSTRUCCION,
	PAQUETE,
    PCB
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct
{
	int pid;
	t_instruccion* instruccion;
	int program_counter;

	t_registros* registros_CPU;

} t_contexto_ejec;

extern t_log* logger;

void* recibir_buffer(int*, int);
char* recibir_buffer2(int* size, int socket_module_cliente);

//CONEXIONES
int iniciar_servidor(t_log *logger, char* ip, char* puerto);
int esperar_cliente(t_log *logger, int socket_module_server);
int crear_conexion(t_log *logger, char* ip, char* puerto, char *modulo);
void liberar_conexion(int socket_module_cliente);

//MENSAJES
void enviar_mensaje(char* mensaje, int socket_module_cliente);
void recibir_mensaje(t_log *logger, int);
int recibir_operacion(int);
int enviar_path(int, const char*);
char* recibir_path(int);
void recibir_pc(int socket_module_cliente, int *pid, int* pc);
void espera(int mls);

//PAQUETES
t_paquete* crear_paquete(op_code codigo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_module_cliente);
t_list* recibir_paquete(int);
void eliminar_paquete(t_paquete* paquete);
t_paquete* serializar_pcb(t_pcb*);
t_pcb* deserializar_paquete_pcb(t_paquete* paquete);
t_paquete* recibir_PCB(int socket);
void recibir_path_y_pid(int socket_module_cliente, char **path, int *pid);


#endif /* UTILS_H_ */

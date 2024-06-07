#include <stdlib.h>
#include <stdio.h>
#include <utils/utils.h>
#include <utils/utils.c>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/pcb.h>
#include <utils/pcb.c>



char* ip_kernel, datos;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
char* modulo_kernel;
char* mensaje;
char* modulo_memoria;
char*puerto_cpu_dispatch;
char*puerto_cpu_interrupt;
t_registros* registros_cpu;
t_pcb* pcb;
void* recibir_kernel_dispatch(void* arg) {
    t_log* logger = (t_log*) arg;
    int cpu_listening_kernel_dispatch = iniciar_servidor(logger, ip_kernel, puerto_cpu_dispatch);
    log_info(logger, "Servidor DISPATCH listo para recibir al kernel");
    int kernel_wait_dispatch = esperar_cliente(logger, cpu_listening_kernel_dispatch);

while (1) {
     int cod_op = recibir_operacion(kernel_wait_dispatch);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(logger, kernel_wait_dispatch);
                break;
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                pthread_exit(NULL);
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
    }

    while (1) {
		t_paquete* paquete =  recibir_paquete(cpu_listening_kernel_dispatch);
         pcb = deserializar_paquete_pcb(paquete);
		log_info(logger, "Recibí el PCB con PID: %s", pcb);
		//Acá se podría agregar el semaforo para que se ejecute el hilo que lea el path y levante las instrucciones
    }
    //aca hay que pasar el pcb a registros (eso en decode)
    }
}

void* recibir_kernel_interrupt(void* arg) {
    t_log* logger = (t_log*) arg;
    int cpu_listening_kernel_interrupt = iniciar_servidor(logger, ip_kernel, puerto_cpu_interrupt);
    log_info(logger, "Servidor INTERRUPT listo para recibir al kernel");
    int kernel_wait_interrupt = esperar_cliente(logger, cpu_listening_kernel_interrupt);
    while (1) {
        int cod_op = recibir_operacion(kernel_wait_interrupt);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(logger, kernel_wait_interrupt);
                break;
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                pthread_exit(NULL);
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
    }
}

void* conectar_a_memoria(void* arg){
    t_log* logger = (t_log*) arg;
    if(logger == NULL) {
        perror("Error al crear el logger");
        pthread_exit(NULL);
    }

    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria, modulo_memoria);
    enviar_mensaje(mensaje, conexion_memoria);
    log_info(logger, "Conexión con memoria realizada correctamente");

    
}



    
int main(int argc, char* argv[]) {
registros_cpu = (t_registros*)malloc(sizeof(t_registros));
if (registros_cpu == NULL) {
// Manejar error de asignación de memoria
    perror("Error al asignar memoria para registros_cpu");
    exit(EXIT_FAILURE);
}
pcb = (t_pcb*)malloc(sizeof(t_pcb));
if (pcb == NULL) {
    // Manejar error de asignación de memoria
    perror("Error al asignar memoria para pcb");
    exit(EXIT_FAILURE);
}

pthread_t  conexion_kernel_dispatch, conexion_memoria_id, conexion_kernel_interrupt;
t_config*config;
t_log* logger;

logger=log_create("/home/utnso/Desktop/tp-2024-1c-GFALT/cpu/cpu.log","cpu_main",1,LOG_LEVEL_INFO);
if(logger==NULL){
    perror("algo paso con el loger");
    exit(EXIT_FAILURE);
}
log_info(logger,"cpu funcionando OK, configuracion en lectura OK.");

config=config_create("/home/utnso/Desktop/tp-2024-1c-GFALT/cpu/cpu.config");
if(config==NULL){
    perror("la configuracion no fue leida correctamente.");
}
ip_kernel=config_get_string_value(config,"IP_KERNEL");
modulo_kernel=config_get_string_value(config,"MODULO_MEMORIA");
ip_memoria=config_get_string_value(config,"IP_MEMORIA");
puerto_memoria=config_get_string_value(config,"PUERTO_MEMORIA");
puerto_cpu_dispatch=config_get_string_value(config,"PUERTO_CPU_DISPATCH");
puerto_cpu_interrupt=config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
mensaje=config_get_string_value(config,"MENSAJE");

log_info(logger,"ips y puertos de kernel y memoria leido correctamente");

//conexion a memoria

pthread_create(&conexion_memoria_id, NULL, conectar_a_memoria, logger);

//conexion a kernel

pthread_create(&conexion_kernel_dispatch, NULL, recibir_kernel_dispatch, logger);
pthread_create(&conexion_kernel_interrupt, NULL, recibir_kernel_interrupt, logger);



pthread_join(conexion_kernel_dispatch, NULL);
pthread_join(conexion_kernel_interrupt, NULL);



// int valor_registro= buscarRegistro(/*aca iria instruccion pasada por memoria*/);

//---------------- EXECUTE ----------------------------
/*  void decode(t_instruccion* proxima_instruccion, t_pcb* contexto){
    cod_instruccion codigo_instruccion= proxima_instruccion-> instruccion;
    switch (codigo_instruccion)
    {
    case SET:
    ejecucion_set(proxima_instruccion->parametro1,proxima_instruccion->parametro2,contexto->registros);
    log_info(logger,"se esta ejecutando SET");
    break;

    case SUM:
    SUM(&registro_destino, &registro_origen);
    log_info(logger, "se esta ejecutando SUM");
    break;

    case SUB:

    log_info(logger, "se esta ejecutando SUB");
    break;

    case JNZ:

    log_info(logger, "se esta ejecutando JNZ");
    break;

    case IO_GEN_SLEEP:

    log_info(logger, "se esta ejecutando IO_GEN_SLEEP");
    break;

    default:

    log_error(logger, "instruccion no reconocida :/");
        return;
    }

}*/
//------------------- FIN DE EXECUTE ---------------------



/*
free(registros_cpu);
free(pcb);
return 0;
*/


}
/*

//CICLO DE INSTRUCCIONES

//FETCH
/*


*/
//FIN FETCH


//DECODE
/* 
    

*/
//FIN DECODE


/*
//DESARROLLO DE INSTRUCCIONES 
int buscarRegistro(char registro[3], t_pcb* valores){
        int valor=0;
        if(strcmp(registro,"AX")==0){
        valor= valores->registros.ax;
        }else if(strcmp(registro,"BX")==0){
        valor = valores->registros.bx;
        }else if(strcmp(registro,"CX")==0){
        valor = valores->registros.cx;
        }else if(strcmp(registro,"DX")==0){
        valor = valores->registros.dx;
        }else if(strcmp(registro,"EAX")==0){
        valor = valores->registros.eax;
        }else if(strcmp(registro,"EBX")==0){
        valor = valores->registros.ebx;
        }else if(strcmp(registro,"ECX")==0){
        valor = valores->registros.ecx;
        }else if(strcmp(registro,"EDX")==0){
        valor = valores->registros.edx;   
        }else if(strcmp(registro,"SI")==0){
        valor = valores->registros.si;
        }else if(strcmp(registro,"SD")==0){
        valor = valores->registros.sd; 
        }
        return valor;
    }
// ---------------------------- EJECUCION DE SET -------------------------------
void ejecucion_SET(char* instruccion, t_pcb* registros){
    setear_registro(instruccion, registros);
}

 void setear_registro(char* instruccion, int valor){
        if(strcmp(instruccion,"AX")==0){
        pcbNuevo->registros.ax = valor
        }else if(strcmp(instruccion,"BX")==0){
        pcbNuevo->registros.bx = valor
        }else if(strcmp(instruccion,"CX")==0){
        pcbNuevo->registros.cx = valor
        }else if(strcmp(instruccion,"DX")==0){
        pcbNuevo->registros.dx = valor
        }else if(strcmp(instruccion,"EAX")==0){
        pcbNuevo->registros.eax = valor
        }else if(strcmp(instruccion,"EBX")==0){
        pcbNuevo->registros.ebx = valor
        }else if(strcmp(instruccion,"ECX")==0){
        pcbNuevo->registros.ecx = valor
        }else if(strcmp(instruccion,"EDX")==0){
        pcbNuevo->registros.edx = valor   
        }else if(strcmp(instruccion,"SI")==0){
        pcbNuevo->registros.si = valor
        }else if(strcmp(instruccion,"SD")==0){
        pcbNuevo->registros.sd = valor 
        }
}


//------------------- FIN DE SET ------------------------------

    

//------------- FUNCION SUM ------------------------
void SUM(t_pcb* registros,char*destino[3], char*origen[3]) {
if(strcmp(destino,"AX")==0){
        pcbNuevo->registros.ax += buscarRegistro(origen, registros);
        }else if(strcmp(destino,"BX")==0){
        pcbNuevo->registros.bx += buscarRegistro(origen, registros);
        }else if(strcmp(destino,"CX")==0){
        pcbNuevo->registros.cx += buscarRegistro(origen, registros);
        }else if(strcmp(destino,"DX")==0){
        pcbNuevo->registros.dx += buscarRegistro(origen, registros);
        }else if(strcmp(destino,"EAX")==0){
        pcbNuevo->registros.eax += buscarRegistro(origen, registros);
        }else if(strcmp(origen,"EBX")==0){
        pcbNuevo->registros.ebx += buscarRegistro(origen, registros);
        }else if(strcmp(registros,"ECX")==0){
        pcbNuevo->registros.ecx += buscarRegistro(origen, registros);
        }else if(strcmp(instruccion,"EDX")==0){
        pcbNuevo->registros.edx += buscarRegistro(origen, registros);  
        }else if(strcmp(instruccion,"SI")==0){
        pcbNuevo->registros.si += buscarRegistro(origen, registros);
        }else if(strcmp(instruccion,"SD")==0){
        pcbNuevo->registros.sd += buscarRegistro(origen, registros);
        }
}

//----------------- FUNCION SUB ------------------------------

void SUB(t_pcb* registros,char*destino, char*origen) {
if(strcmp(destino,"AX")==0){
        pcbNuevo->registros.ax -= buscarRegistro(origen, registros);
        }else if(strcmp(destino,"BX")==0){
        pcbNuevo->registros.bx -= buscarRegistro(origen, registros);
        }else if(strcmp(destino,"CX")==0){
        pcbNuevo->registros.cx -= buscarRegistro(origen, registros);
        }else if(strcmp(destino,"DX")==0){
        pcbNuevo->registros.dx -= buscarRegistro(origen, registros);
        }else if(strcmp(destino,"EAX")==0){
        pcbNuevo->registros.eax -= buscarRegistro(origen, registros);
        }else if(strcmp(origen,"EBX")==0){
        pcbNuevo->registros.ebx -= buscarRegistro(origen, registros);
        }else if(strcmp(registros,"ECX")==0){
        pcbNuevo->registros.ecx -= buscarRegistro(origen, registros);
        }else if(strcmp(instruccion,"EDX")==0){
        pcbNuevo->registros.edx -= buscarRegistro(origen, registros);  
        }else if(strcmp(instruccion,"SI")==0){
        pcbNuevo->registros.si -= buscarRegistro(origen, registros);
        }else if(strcmp(instruccion,"SD")==0){
        pcbNuevo->registros.sd -= buscarRegistro(origen, registros);
        }
} */
/*--------------------fin sub---------------------*/


/* -----------------JNZ----------------  */ 
/* 
void JNZ(t_pcb valores, char* registro[3]){

    int valor_registro = buscarRegistro(registro, valores); //busco el valor del registro que necesito 
    if(valor_registro != 0){
        valores->pc = valor_registro;
    }
}  
*/ 

/*------------------IO_GEN_SLEEP-----------------
void IO_GEN_SLEEP(char* tipo_interfaz, int tiempo_unidad_trabajo){
   
   int tiempo_en_micro= tiempo_unidad_trabajo*1000; 

   usleep(tiempo_en_micro); 

}
*/

/*
char* recibir_instruccion_memoria(){
    recibir_operacion(conexion_memoria);
    t_list* lista = list_create();    
    lista = recibir_paquete(conexion_memoria);
    char * instruccion = list_get(lista, 0);
    list_destroy(lista);
    return instruccion;
}

*/

#include "main.h"

int socket_servidor_memoria;
sem_t semaforo_hilo_cpu;
sem_t semaforo_hilo_kernel;
sem_t semaforo_hilo_entradasalida;


void* conexion_CPU(void* arg) {
	sem_wait(&semaforo_hilo_cpu); // Espera el semaforo para el hilo de CPU

    t_log* logger = (t_log*) arg;
    log_info(logger, "[HILO CPU] Se inició hilo CPU");

    int socket_cliente_cpu = esperar_cliente(logger, socket_servidor_memoria);

	sem_post(&semaforo_hilo_kernel); // Libera el semaforo para el hilo de Kernel
    while (1) {
        int cod_op2 = recibir_operacion(socket_cliente_cpu);

        if (cod_op2 == -1) {
            log_error(logger, "Error al recibir operación. Reintentando...");
            continue;
        }

        recibir_mensaje(logger, socket_cliente_cpu);
		log_info(logger, "Hilo CPU recibió el mensaje");
		
    }

    return NULL;
}

void* conexion_Kernel(void* arg) {
	sem_wait(&semaforo_hilo_kernel); // Espera el semaforo para el hilo de Kernel
    t_log* logger = (t_log*) arg;
    log_info(logger, "[HILO KERNEL] Se inició hilo Kernel");

    int socket_cliente_kernel = esperar_cliente(logger, socket_servidor_memoria);

	sem_post(&semaforo_hilo_entradasalida);

	      int cod_op2 = recibir_operacion(socket_cliente_kernel);

        if (cod_op2 == -1) {
            log_error(logger, "Error al recibir operación. Reintentando...");
        }

        recibir_mensaje(logger, socket_cliente_kernel);
		log_info(logger, "Hilo KERNEL recibió el mensaje");

    while (1) {
		char* path = recibir_path(socket_cliente_kernel);	
		log_info(logger, "Recibí el path: %s", path);
		//Acá se podría agregar el semaforo para que se ejecute el hilo que lea el path y levante las instrucciones
    }

    return NULL;
}

void *conexion_entradasalida(void *arg) {
	sem_wait(&semaforo_hilo_entradasalida);

    t_log *logger = (t_log *)arg; // Asumiendo que pasas el logger como argumento

    log_info(logger, "[conexion_entradasalida] Hilo de conexión de entrada/salida iniciado.");

    int socket_cliente_entradasalida = esperar_cliente(logger, socket_servidor_memoria);
    
    sem_post(&semaforo_hilo_kernel); // Libera el semaforo para el hilo de Kernel

    while (1) {
        int cod_op_entradasalida = recibir_operacion(socket_cliente_entradasalida);

        if (cod_op_entradasalida == -1) {
            log_error(logger, "Error al recibir operación. Reintentando...");
            continue;
        }

        recibir_mensaje(logger, socket_cliente_entradasalida);
        log_info(logger, "Hilo Entrada/Salida recibió el mensaje");
    }

    return NULL;
}

void manejar_signal(int signo) {
    // Liberar el socket y otras acciones de limpieza necesarias
    close(socket_servidor_memoria);

    // Salir del programa
    exit(0);
} pthread_t thread_entradasalida;

void instruccion_destroyer(t_instruccion* instruccion){
    free(instruccion->codinstruccion);

    if(instruccion->parametro1_lenght != 0 && instruccion->parametro2_lenght != 0 && instruccion->parametro3_lenght != 0){

    	free(instruccion->parametros[0]);
		free(instruccion->parametros[1]);
		free(instruccion->parametros[2]);

    } else if(instruccion->parametro1_lenght != 0 && instruccion->parametro2_lenght != 0 ){

    	free(instruccion->parametros[0]);
		free(instruccion->parametros[1]);

    } else if(instruccion->parametro1_lenght != 0 ){

    	free(instruccion->parametros[0]);

    }

    free(instruccion);
}

int main(void) {

	signal(SIGINT, manejar_signal);
	
	int socket_cliente_memoria;

	sem_init(&semaforo_hilo_cpu, 0, 1); // Inicializa el semaforo para el hilo de CPU
	sem_init(&semaforo_hilo_kernel, 0, 0); // Inicializa el semaforo para el hilo de Kernel
	sem_init(&semaforo_hilo_entradasalida, 0, 0);


	pthread_t hilo_cpu;
	pthread_t hilo_kernel;
	pthread_t hilo_entradasalida;


	logger = iniciar_logger();
	log_info(logger,"[iniciar_logger] Logger creado OK.");

	config = iniciar_config();
	log_info(logger,"[iniciar_config] Configuración creada OK.");

	// CARGO CONFIG

    leer_config();
	log_info(logger,"[leer_config] Configuración cargada OK.");

    socket_servidor_memoria = iniciar_servidor(logger, IP, PUERTO_ESCUCHA);
	log_info(logger, "[iniciar_servidor] Servidor iniciado OK");

	pthread_create(&hilo_cpu, NULL, conexion_CPU, (void*) logger);
    pthread_create(&hilo_kernel, NULL, conexion_Kernel, (void*) logger);
	pthread_create(&hilo_entradasalida, NULL, conexion_entradasalida, (void*) logger);
	

	pthread_join(hilo_cpu, NULL);
	pthread_join(hilo_kernel, NULL);
	pthread_join(hilo_entradasalida, NULL);
	
	
	return 0;
}

void leer_config(){

	IP = config_get_string_value(config, "IP_ESCUCHA");
	log_info(logger,"La Ip es:  %s",IP);

	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	log_info(logger,"El puerto de escucha es: %s",PUERTO_ESCUCHA);

	TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
	TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");
	PATH_INSTRUCCIONES = config_get_int_value(config, "PATH_INSTRUCCIONES");
	RETARDO_RESPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");

	if(!IP || !PUERTO || !TAM_MEMORIA || !TAM_PAGINA || !PATH_INSTRUCCIONES || !RETARDO_RESPUESTA){

		log_error(logger,"[leer_config] ¡No se pudieron recibir los datos del archivo de configuracion!\n");
		terminar_programa(logger, config);

	}
}


t_log* iniciar_logger(void)
{
	t_log* logger_memoria;

	if ((logger_memoria = log_create("memoria.log", "Memoria_Main", 1, LOG_LEVEL_INFO)) == NULL) {
		perror("[iniciar_logger] ¡No se pudo crear el logger!\n");
		exit(EXIT_FAILURE);
	}

	return logger_memoria;
}

t_config* iniciar_config(void)
{
	t_config* config_memoria = config_create ("/home/utnso/Desktop/tp-2024-1c-GFALT/memoria/memoria.config");

	if (config_memoria == NULL) {
		perror("[iniciar_config] ¡No se pudo cargar el config!\n");
		exit(EXIT_FAILURE);
	}

	return config_memoria;

}

void terminar_programa(t_log *logger, t_config *config) {
	log_destroy(logger);
	config_destroy(config);
	close(socket_servidor_memoria);
}


void leer_archivopseudocode(int socket_cliente_cpu){

	char* path_file;
	int pid;

	recibir_path_y_pid(socket_cliente_cpu, &path_file, &pid);

	char* path = string_new();
	string_append(&path, PATH_INSTRUCCIONES);
	string_append(&path, "/");
	string_append(&path, path_file);

	if(string_contains(path, "./")){
		char* buffer = malloc(100*sizeof(char));
		getcwd(buffer, 100); 
		string_append(&buffer, "/"); 
		path = string_replace(path, "./", buffer);
	}else if(string_contains(path, "~/")){
		path = string_replace(path, "~/", "/home/utnso/");
	}

	FILE* archivo = fopen(path,"r");


	//Se comprueba la existencia del archivo
	if(archivo == NULL){
		log_error(logger, "¡No se pudo abrir el archivo! Error: %d (%s)", errno, strerror(errno));
		free(path);
		return;
	}

	//Variables locales

	char* cadena;
	t_list* lista_instrucciones = list_create();

	//Lectura archivo
	while(feof(archivo) == 0)
	{

		cadena = malloc(300);
		char *resultado_cadena = fgets(cadena, 300, archivo);

		//si el char esta vacio, hace break.
		if(resultado_cadena == NULL)
		{
			log_error(logger, "¡El archivo esta vacio! Error: %d (%s)", errno, strerror(errno));
			break;
		}

		// se borra los '\n'
		if(string_contains(cadena, "\n")){
			char** array_de_cadenas = string_split(cadena, "\n");

			cadena = string_array_pop(array_de_cadenas);


			while(strcmp(cadena, "") == 0){
				cadena = string_array_pop(array_de_cadenas);
			}

			string_array_destroy(array_de_cadenas);
		}

		//Creacion de Instruccion
		t_instruccion *ptr_inst = malloc(sizeof(t_instruccion));

		//Lectura codigo operacion
		char* token = strtok(cadena," ");
		ptr_inst->codinstruccion = token;

		ptr_inst->instruccion_lenght = strlen(ptr_inst->codinstruccion) + 1;


		ptr_inst->parametros[0] = NULL;
		ptr_inst->parametros[1] = NULL;
		ptr_inst->parametros[2] = NULL;

		//Lectura parametros
		token = strtok(NULL, " ");
		int n = 0;
		while(token != NULL)
		{
			ptr_inst->parametros[n] = token;
			token = strtok(NULL, " ");
			n++;
		}

		if(ptr_inst->parametros[0] != NULL){
			ptr_inst->parametro1_lenght = strlen(ptr_inst->parametros[0])+1;
		} else {
			ptr_inst->parametro1_lenght = 0;
		}
		if(ptr_inst->parametros[1] != NULL){
			ptr_inst->parametro2_lenght = strlen(ptr_inst->parametros[1])+1;
		} else {
			ptr_inst->parametro2_lenght = 0;
		}
		if(ptr_inst->parametros[2] != NULL){
			ptr_inst->parametro3_lenght = strlen(ptr_inst->parametros[2])+1;
		} else {
			ptr_inst->parametro3_lenght = 0;
		}

		list_add(lista_instrucciones,ptr_inst);

	}

	dictionary_put(lista_instruccion_pid, string_itoa(pid),lista_instrucciones);

	enviar_mensaje("Archivo leido OK", socket_cliente_cpu);

	free(path);
	fclose(archivo);
}

void enviar_instruccion_a_cpu(int socket_cliente_cpu,int RETARDO_RESPUESTA)
{

	int pc ;
	int pid ;

	recibir_pc(socket_cliente_cpu, &pid, &pc);

	t_list* lista_instrucciones = dictionary_get(lista_instruccion_pid,string_itoa(pid));

	if(lista_instrucciones == NULL){
		log_error(logger, "¡No se encontro ninguna lista de isntrucciones! Error: %d (%s)", errno, strerror(errno));
	}

	//Se busca la instruccion
	t_instruccion *instruccion = list_get(lista_instrucciones,pc-1);

	//Se crea y se serializa el paquete
	t_paquete *paquete_instruccion = crear_paquete(INSTRUCCION);

	agregar_a_paquete(paquete_instruccion,instruccion->codinstruccion,instruccion->instruccion_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[0],instruccion->parametro1_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[1],instruccion->parametro2_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[2],instruccion->parametro3_lenght);

	espera(RETARDO_RESPUESTA);

	enviar_paquete(paquete_instruccion,socket_cliente_cpu);

	eliminar_paquete(paquete_instruccion);
}



void iterator(char* value, t_log* logger) {
	log_info(logger,"%s", value);
}




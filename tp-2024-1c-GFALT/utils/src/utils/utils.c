#include "utils.h"
#include "pcb.h"

//t_log* logger;


int crear_conexion(t_log *logger, char *ip, char *puerto, char *modulo) {
	
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_module_cliente = socket(server_info->ai_family,
			server_info->ai_socktype, server_info->ai_protocol);

	int conexion_cliente = connect(socket_module_cliente, server_info->ai_addr,
			server_info->ai_addrlen);

	// Ahora que tenemos el socket, vamos a conectarlo
	while (conexion_cliente == -1) {
		log_info(logger, "[crear_conexion] No se pudo conectar al modulo %s. Reintentando en 2 segundos.",
				modulo);
		sleep(2);
		conexion_cliente = connect(socket_module_cliente, server_info->ai_addr,
				server_info->ai_addrlen);
	}

	freeaddrinfo(server_info);

	return socket_module_cliente;
}

int iniciar_servidor(t_log *logger, char* ip, char* puerto) {

	log_info(logger, "[iniciar_servidor] Se inicia la apertura del Servidor");

	//SE CREA VARIABLE PARA GUARDAR EL SOCKET DE MEMORIA PARA ESCUCHA
	
	int socket_module_server;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	//SE ASGINA IP Y PUERTO

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// SE CREA EL SOCKET DE ESCUCHA PARA EL SERVIDOR

	 socket_module_server = socket(servinfo->ai_family,
								servinfo->ai_socktype,
								servinfo->ai_protocol);


	// Permitir la reutilización de la dirección y puerto
    int yes = 1;
    if (setsockopt(socket_module_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }							

	// ASOCIAMOS EL SOCKET CON LOS PARAMETROS INGRESADOS
	
	if (bind(socket_module_server,servinfo->ai_addr,servinfo->ai_addrlen) != 0){
		perror("[iniciar_servidor] Falló el bind");
		exit(1);
	}

	log_info(logger, "[iniciar_servidor] Bind existoso");

	log_trace(logger,"[iniciar_servidor] A la espera de escuchar al modulo entrante");

	// ESCUCHAMOS LAS CONEXIONES ENTRANTES

	listen(socket_module_server, SOMAXCONN);

	freeaddrinfo(servinfo);
    
	return socket_module_server;

}

int esperar_cliente(t_log *logger, int socket_module_server)
{

	// Aceptamos un nuevo cliente
	int socket_module_cliente = accept(socket_module_server, NULL, NULL);
	log_info(logger, "[esperar_cliente] Se conecto un Modulo en socket %d",socket_module_cliente);

	return socket_module_cliente;

}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_module_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_module_cliente, a_enviar, bytes, 0);

	free(a_enviar);

	eliminar_paquete(paquete);

}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}


void recibir_mensaje(t_log *logger, int socket_module_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_module_cliente);
	log_info(logger, "Me llego el mensaje <%s> en el socket %d", buffer, socket_module_cliente);
	free(buffer);

}


int enviar_path(int socket, const char* str) {
    int len = strlen(str) + 1; // +1 para incluir el carácter nulo
    int bytes_sent = send(socket, &len, sizeof(int), 0);
    if (bytes_sent <= 0) {
        // manejar error
        return -1;
    }
    bytes_sent = send(socket, str, len, 0);
    if (bytes_sent <= 0) {
        // manejar error
        return -1;
    }
    return 0; // éxito
}

char* recibir_path(int socket) {
    int len;
    int bytes_received = recv(socket, &len, sizeof(int), MSG_WAITALL);
    if (bytes_received <= 0) {
        // manejar error o conexión cerrada
        return NULL;
    }
    char* str = malloc(len);
    if (str == NULL) {
        // manejar error de malloc
        return NULL;
    }
    bytes_received = recv(socket, str, len, MSG_WAITALL);
    if (bytes_received < len) {
        // manejar error o conexión cerrada
        free(str);
        return NULL;
    }
    return str; // el llamador es responsable de liberar str
}

void* recibir_buffer(int* size, int socket_module_cliente)
{
	void * buffer;

	recv(socket_module_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_module_cliente, buffer, *size, MSG_WAITALL);

	return buffer;

}


char* recibir_buffer2(int* size, int socket_module_cliente)
{
    char * buffer;

    int bytes_read = recv(socket_module_cliente, size, sizeof(int), MSG_WAITALL);
    if (bytes_read <= 0) {
        // handle error or closed connection
        return NULL;
    }

    buffer = malloc(*size + 1); // +1 for the null terminator
    if (buffer == NULL) {
        // handle failed malloc
        return NULL;
    }

    bytes_read = recv(socket_module_cliente, buffer, *size, MSG_WAITALL);
    if (bytes_read < *size) {
        // handle error or closed connection
        free(buffer);
        return NULL;
    }

    buffer[*size] = '\0'; // ensure the string is null-terminated

    return buffer;
}

void liberar_conexion(int socket_module_cliente)
{
	close(socket_module_cliente);
}

t_list* recibir_paquete(int socket_module_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_module_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

int recibir_operacion(int socket_module_cliente) {
    int cod_op;
    
    // Verificar si la conexión sigue activa
    int result = recv(socket_module_cliente, &cod_op, sizeof(int), MSG_PEEK);
    if (result == 0) {
        // El otro extremo ha cerrado la conexión
        close(socket_module_cliente);
        return -1;
    } else if (result < 0) {
        // Error al recibir datos
        close(socket_module_cliente);
        return -1;
    }
    
    // Continuar recibiendo la operación
    result = recv(socket_module_cliente, &cod_op, sizeof(int), MSG_WAITALL);
    if (result > 0) {
        return cod_op;
    } else {
        close(socket_module_cliente);
        return -1;
    }
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code codigo_operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_operacion;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_module_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_module_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}
/*
void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
*/
cod_instruccion instruccion_to_enum(char* instruccion){

	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if(strcmp(instruccion, "MOV_OUT") == 0){
		return MOV_OUT;
	}else if(strcmp(instruccion, "SUM") == 0){
		return SUM;
	} else if(strcmp(instruccion, "SUB") == 0){
		return SUB;
	} else if(strcmp(instruccion, "JNZ") == 0){
		return JNZ;
	} else if(strcmp(instruccion, "COPY_STRING") == 0){
		return COPY_STRING;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;
	} else if(strcmp(instruccion, "IO_GEN_SLEEP") == 0){
		return IO_GEN_SLEEP;
	} else if(strcmp(instruccion, "IO_STDIN_READ") == 0){
		return IO_STDIN_READ;
	} else if(strcmp(instruccion, "IO_STDOUT_WRITE") == 0){
		return IO_STDOUT_WRITE;
	} else if(strcmp(instruccion, "IO_FS_CREATE") == 0){
		return IO_FS_CREATE;
	} else if(strcmp(instruccion, "IO_FS_DELETE") == 0){
		return IO_FS_DELETE;
	} else if(strcmp(instruccion, "IO_FS_TRUNCATE") == 0){
		return IO_FS_TRUNCATE;
	} else if(strcmp(instruccion, "IO_FS_WRITE") == 0){
		return IO_FS_WRITE;
	} else if(strcmp(instruccion, "IO_FS_READ") == 0){
		return IO_FS_READ;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	}
	return -1;
}





//

t_paquete* serializar_pcb(t_pcb* pcb) {
    // Obtener el tamaño del path directamente de pcb->path
    int path_size = strlen(pcb->path) + 1; // +1 para el carácter nulo

    // Calcular el tamaño total del paquete serializado
    int size = sizeof(pcb->pid) + sizeof(pcb->pc) + sizeof(int) + path_size + sizeof(pcb->quantum) + sizeof(pcb->estado) + sizeof(pcb->registros);

    // Crear el paquete
    void* stream = malloc(size);
    if (stream == NULL) {
        // manejar error de malloc
        return NULL;
    }

    // Copiar los datos en el paquete
    int offset = 0;
    memcpy(stream + offset, &(pcb->pid), sizeof(pcb->pid));
    offset += sizeof(pcb->pid);
    memcpy(stream + offset, &(pcb->pc), sizeof(pcb->pc));
    offset += sizeof(pcb->pc);
    memcpy(stream + offset, &path_size, sizeof(int)); // tamaño del path
    offset += sizeof(int);
    memcpy(stream + offset, pcb->path, path_size); // path
    offset += path_size;
    memcpy(stream + offset, &(pcb->quantum), sizeof(pcb->quantum));
    offset += sizeof(pcb->quantum);
    memcpy(stream + offset, &(pcb->estado), sizeof(pcb->estado));
    offset += sizeof(pcb->estado);
    memcpy(stream + offset, &(pcb->registros), sizeof(pcb->registros));

    // Crear y devolver la estructura t_buffer
    t_buffer* buffer = malloc(sizeof(t_buffer));
    if (buffer == NULL) {
        // manejar error de malloc
        free(stream);
        return NULL;
    }
    buffer->size = size;
    buffer->stream = stream;

    // Crear y devolver la estructura t_paquete
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (paquete == NULL) {
        // manejar error de malloc
        free(buffer);
        return NULL;
    }
    paquete->codigo_operacion = PCB;
    paquete->buffer = buffer;

    // Crear el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
    offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    return paquete; // el llamador es responsable de liberar paquete
}



int enviar_pcb(int socket, t_paquete* paquete) {
    // Calcular el tamaño total del paquete a enviar
    int total_size = sizeof(uint8_t) + sizeof(uint32_t) + paquete->buffer->size;

    // Crear el stream a enviar
    void* a_enviar = malloc(total_size);
    if (a_enviar == NULL) {
        // manejar error de malloc
        return -1;
    }

    // Copiar los datos en el stream a enviar
    int offset = 0;
    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Enviar el stream
    int sent_bytes = send(socket, a_enviar, total_size, 0);
    if (sent_bytes != total_size) {
        // manejar error de send
        free(a_enviar);
        return -1;
    }

    // Liberar la memoria que ya no usaremos
    free(a_enviar);

    return 0; // éxito
}


t_paquete* recibir_PCB(int socket) {
    // Recibir el código de operación
    uint8_t codigo_operacion;
    int received_bytes = recv(socket, &codigo_operacion, sizeof(uint8_t), 0);
    if (received_bytes != sizeof(uint8_t)) {
        // manejar error de recv
        return NULL;
    }

    // Recibir el tamaño del buffer
    uint32_t buffer_size;
    received_bytes = recv(socket, &buffer_size, sizeof(uint32_t), 0);
    if (received_bytes != sizeof(uint32_t)) {
        // manejar error de recv
        return NULL;
    }

    // Recibir el buffer
    void* buffer_stream = malloc(buffer_size);
    if (buffer_stream == NULL) {
        // manejar error de malloc
        return NULL;
    }
    received_bytes = recv(socket, buffer_stream, buffer_size, 0);
    if (received_bytes != buffer_size) {
        // manejar error de recv
        free(buffer_stream);
        return NULL;
    }

    // Crear el buffer
    t_buffer* buffer = malloc(sizeof(t_buffer));
    if (buffer == NULL) {
        // manejar error de malloc
        free(buffer_stream);
        return NULL;
    }
    buffer->size = buffer_size;
    buffer->stream = buffer_stream;

    // Crear el paquete
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (paquete == NULL) {
        // manejar error de malloc
        free(buffer);
        return NULL;
    }
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    return paquete; // el llamador es responsable de liberar paquete
}


t_pcb* deserializar_paquete_pcb(t_paquete* paquete) {
    // Verificar que el código de operación es PCB
    if (paquete->codigo_operacion != PCB) {
        printf("El paquete no contiene un PCB.\n");
        return NULL;
    }

    // Crear un stream a partir del buffer
    void* stream = paquete->buffer->stream;

    // Crear un PCB
    t_pcb* pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL) {
        // manejar error de malloc
        return NULL;
    }

    // Deserializar los datos del PCB
    int offset = 0;
    memcpy(&(pcb->pid), stream + offset, sizeof(pcb->pid));
    offset += sizeof(pcb->pid);
    memcpy(&(pcb->pc), stream + offset, sizeof(pcb->pc));
    offset += sizeof(pcb->pc);
    int path_size;
    memcpy(&path_size, stream + offset, sizeof(int)); // tamaño del path
    offset += sizeof(int);
    pcb->path = malloc(path_size);
    if (pcb->path == NULL) {
        // manejar error de malloc
        free(pcb);
        return NULL;
    }
    memcpy(pcb->path, stream + offset, path_size); // path
    offset += path_size;
    memcpy(&(pcb->quantum), stream + offset, sizeof(pcb->quantum));
    offset += sizeof(pcb->quantum);
    memcpy(&(pcb->estado), stream + offset, sizeof(pcb->estado));
    offset += sizeof(pcb->estado);
    memcpy(&(pcb->registros), stream + offset, sizeof(pcb->registros));

    return pcb; // el llamador es responsable de liberar pcb
}

void recibir_pc(int socket_module_cliente, int *pid,int* pc ){
	int size;
	int desplazamiento = 0;
	void * buffer;

	buffer = recibir_buffer(&size, socket_module_cliente);
	while(desplazamiento < size)
	{
		memcpy(pc, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(pid, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
	}
	free(buffer);

}

void recibir_path_y_pid(int socket_module_cliente, char **path, int *pid)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	int tamanio;

	buffer = recibir_buffer(&size, socket_module_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		*path = malloc(tamanio);

		memcpy(*path, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;

		memcpy(pid, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
	}
	free(buffer);
}

void espera(int mls){
	// el * 1000 es para pasarlo a microsegundos
	usleep(mls*1000);
}

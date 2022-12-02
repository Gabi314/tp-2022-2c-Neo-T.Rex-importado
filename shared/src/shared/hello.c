#include <shared/hello.h>

t_log* logger;


int say_hello(char* who) {
    return printf("Hello %s!!\n", who);
}

t_log* iniciar_logger(char* file, char* logName) {
    t_log* nuevo_logger = log_create(file, logName, 1, LOG_LEVEL_DEBUG );
    return nuevo_logger;
}

void enviar_entero(int valor, int socket_cliente, int cod_op) {
	send(socket_cliente, &cod_op, sizeof(int), 0);
	send(socket_cliente, &valor, sizeof(int), 0);
}

t_list* recibir_lista_enteros(int socket_cliente) // me base en el recibir paquete del tp0
{

	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		int valor = 0;
		memcpy(&valor, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		list_add(valores, (void *)valor);
	}
	free(buffer);
	return valores;
}

int recibir_entero(int socket_cliente) // basado en recibir_operacion
{
	int entero;
	recv(socket_cliente, &entero, sizeof(int), MSG_WAITALL);
	log_info(logger,"[recibir_entero]: se recibio el numero %d",entero);
	return entero;

}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
		if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
			return cod_op;
		else
		{
			close(socket_cliente);
			return -1;
		}
}

void * recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

char* recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	return buffer;
	free(buffer);
}



void* serializar_paquete(t_paquete* paquete, int bytes) // sirve para cualquier estructura paquete
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

int iniciar_servidor(char* ip, char* puerto, char* cliente)
{
	int socket_servidor;

	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(server_info);

	log_trace(logger, "Listo para escuchar a %s", cliente);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor, char* cliente) {
	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,(void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Se conecto el %s", cliente);

	return socket_cliente;
}


int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Crear el socket
	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	// Fallo al crear el socket
	if (socket_cliente == -1) {
		// Pasar logger correspondiente por parametro
		//log_error(logger, "Hubo un error al crear el socket del servidor");
		return 0;
	}

	// Conectar el socket...
	// Error al conectar
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
		//log_error(logger, "Error al conectar al servidor);
		freeaddrinfo(server_info);
		return 0;
	} else {
		// Conexion exitosa
		//log_info(logger, "Cliente conectado");
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}


void enviar_mensaje(char* mensaje, int socket_cliente, int cod_op) //podríamos usar esto polimorficamente con cualquier mensaje para no
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = cod_op;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete) // inicializa el paquete sin codigo de operacion
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


t_paquete* crear_paquete(int cod_op) // inicializa el paquete con codigo de operacion
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete_unInt(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

char** recibirListaDispositivos(int conexion){
	log_info(logger,"llego hasta recibir lista dispositivos");
	int cod_op = recibir_operacion(conexion);
	char** listaDispositivos;

	if(cod_op == KERNEL_MENSAJE_DISPOSITIVOS_IO){
		char* dispositivos_IO = recibir_mensaje(conexion);
		listaDispositivos = string_split(dispositivos_IO,"-");
		log_info(logger,"Primer dispositivo %s",listaDispositivos[0]);

	}
	else {
		log_info(logger,"codigo de operacion incorrecto");
	}

	return listaDispositivos;
}

void iteratorMostrarInstrucciones(instruccion* instruccion){
	log_info(logger,"%s param1: %d param2: %d", instruccion->identificador, instruccion->parametros[0],instruccion->parametros[1]);
}

void controlar_pcb(t_pcb * PCB){
	log_info(logger,"------------------------DATOS DEL PCB----------------------------");
	log_info(logger,"pid: %d",PCB->idProceso);
	log_info(logger,"Instrucciones:");
	list_iterate(PCB->instrucciones, (void*) iteratorMostrarInstrucciones);
	log_info(logger,"program counter en posicion: %d",PCB->programCounter);
	t_registros  registro = PCB->registros;
	log_info(logger,"registro AX: %d", registro.AX);
	log_info(logger,"registro BX: %d", registro.BX);
	log_info(logger,"registro CX: %d", registro.CX);
	log_info(logger,"registro DX: %d", registro.DX);

	for(int i=0;i<list_size(PCB->tablaSegmentos);i++) {
		entradaTablaSegmento * elemento = list_get(PCB->tablaSegmentos,i);
		log_info(logger,"posicion %d de la tabla de segmentos |NUMERO DEL SEGMENTO:%d |TAMANIO DEL SEGMENTO: %d |NUMERO DE TABLA DE PAGINAS: %d", i,elemento->numeroSegmento,elemento->tamanioSegmento, elemento->numeroTablaPaginas);
	}
	log_info(logger,"numero de socket/consola: %d", PCB->socket);


	switch(PCB->estado){
	case NEW:
		log_info(logger,"estado: NEW");
		break;
	case READY:
		log_info(logger,"estado: READY");
		break;
	case BLOCKED:
		log_info(logger,"estado: BLOCKED");
		break;
	case EXEC:
		log_info(logger,"estado: EXEC");
		break;
	case TERMINATED:
		log_info(logger,"estado: TERMINATED");
		break;
	default:
		log_info(logger,"estado invalido");
		break;
	}

	switch(PCB->algoritmoActual){
		case FIFO:
			log_info(logger,"algoritmo: FIFO");
			break;
		case RR:
			log_info(logger,"algoritmo: ROUND ROBIN");
			break;
		default:
			log_info(logger,"algoritmo invalido");
			break;

		}

	log_info(logger,"------------------------FIN DATOS DEL PCB----------------------------");

}

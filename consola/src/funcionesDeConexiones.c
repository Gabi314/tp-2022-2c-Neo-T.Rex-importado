#include "funcionesConsola.h"

//---------------------------------FUNCIONES PARA ENVIAR PAQUETES--------------------------------
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

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente,server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente, int cod_op) //podríamos usar esto polimorficamente con cualquier mensaje
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

// por ahora dejamos esto acá, capaz nos sirva después


t_paquete* crear_paquete(int cod_op) // inicializa el paquete con codigo de operacion
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete_instrucciones(t_paquete* paquete, instruccion* instruccion, int identificador_length)
{
	void* id = instruccion->identificador;
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + identificador_length + sizeof(int) + sizeof(int[2]));;

	memcpy(paquete->buffer->stream + paquete->buffer->size, &identificador_length, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), id, identificador_length);
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int) + identificador_length, &instruccion->parametros, sizeof(int[2]));

	paquete->buffer->size += identificador_length + sizeof(int) + sizeof(int[2]);
}

/*
void agregar_a_paquete_instrucciones(t_paquete* paquete, instruccion* unaInstruccion, int identificador_length)
{
	void* id = unaInstruccion->identificador;
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + identificador_length + sizeof(int) + sizeof(parametro));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &identificador_length, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), id, identificador_length);
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int) + identificador_length, &unaInstruccion->parametros, sizeof(parametro));

	paquete->buffer->size += identificador_length + sizeof(int) + sizeof(parametro);
}
*/
void agregar_a_paquete_segmentos(t_paquete* paquete, void* valor, int tamanio)
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

//-------------------------------------FUNCIONES PARA RECIBIR PAQUETES-----------------------------------------

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

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
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

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

int recibir_entero(int socket_cliente) // basado en recibir_operacion
{
	int entero;
	recv(socket_cliente, &entero, sizeof(int), MSG_WAITALL);
	log_info(logger,"[recibir_entero]: se recibio el numero %d",entero);
	return entero;

}

void enviar_entero(int valor, int socket_cliente, int cod_op) {
	send(socket_cliente, &cod_op, sizeof(int), 0);
	send(socket_cliente, &valor, sizeof(int), 0);
}

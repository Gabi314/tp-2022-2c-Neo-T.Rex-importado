#include "funcionesKernel.h"

int tamanioTotalIdentificadores = 0;
int contadorInstrucciones = 0;
int contadorSegmentos = 0;
int desplazamiento = 0;
t_paquete* paquete;

void enviar_entero(int valor, int socket_cliente, int cod_op) {
	send(socket_cliente, &cod_op, sizeof(int), 0);
	send(socket_cliente, &valor, sizeof(int), 0);
} //probemos esto


t_list* recibir_lista_instrucciones(int socket_cliente) {

int size;
	int desplazamiento = 0;
	void * buffer;

	t_list* listaDeInstrucciones = list_create();

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instruccion* unaInstruccion = malloc(sizeof(instruccion));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		unaInstruccion->identificador = malloc(tamanioIdentificador);
		memcpy(unaInstruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(parametro));
		desplazamiento+=sizeof(parametro);
		list_add(listaDeInstrucciones, unaInstruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones
	}
	free(buffer);
	return listaDeInstrucciones;
}


t_list* recibir_paquete_instrucciones(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	t_list* listaDeInstrucciones = list_create();

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instruccion* unaInstruccion = malloc(sizeof(instruccion));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		unaInstruccion->identificador = malloc(tamanioIdentificador);
		memcpy(unaInstruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(listaDeInstrucciones, unaInstruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones
	}
	free(buffer);
	return listaDeInstrucciones;
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
		list_add(valores, (void *) valor);
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

void recibir_mensaje(int socket_cliente)
{
int size;
char* buffer = recibir_buffer(&size, socket_cliente);
log_info(logger, "Me llego el mensaje %s", buffer);
free(buffer);
}

char * recibir_cadena(int socket_cliente)
{
int size;
char* buffer = recibir_buffer(&size, socket_cliente);
log_info(logger, "Me llego la cadena %s", buffer);
return buffer;
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

int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP_KERNEL, PUERTO_KERNEL, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a consola");

	return socket_servidor;
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
	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente,server_info->ai_addr,server_info->ai_addrlen);

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



// por ahora dejamos esto acá, capaz nos sirva después
/*
t_paquete* crear_super_paquete(void)
{
	//me falta un malloc!
	t_paquete* paquete;

	//descomentar despues de arreglar
	//paquete->codigo_operacion = PAQUETE;
	//crear_buffer(paquete);
	return paquete;
}
*/
t_paquete* crear_paquete(int cod_op) // inicializa el paquete con codigo de operacion
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = cod_op;
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

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}
void agregar_a_paquete_instrucciones(t_paquete* paquete, instruccion* instruccion, int identificador_length)
{
	void* id = instruccion->identificador;
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + identificador_length + sizeof(int) + sizeof(int[2]));;
	if(!strcmp(instruccion->identificador,"EXIT")){
		log_info(logger,"El primer parametro de EXIT es: %d",instruccion->parametros[0]);
		log_info(logger,"El segundo parametro de EXIT es: %d",instruccion->parametros[1]);
	}

	memcpy(paquete->buffer->stream + paquete->buffer->size, &identificador_length, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), id, identificador_length);
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int) + identificador_length, &instruccion->parametros, sizeof(int[2]));

	paquete->buffer->size += identificador_length + sizeof(int) + sizeof(int[2]);
}


t_pcb* recibir_pcb(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = list_create();
	pcb->tabla_segmentos = list_create();
	int contadorInstrucciones = 0;
	int contadorSegmentos = 0;
	int i = 0;
	memcpy(&pcb->idProceso, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);

	log_info(logger,"PID: %d",pcb->idProceso);
	memcpy(&contadorInstrucciones, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	while(i < contadorInstrucciones){
		instruccion* unaInstruccion = malloc(sizeof(instruccion));
		int identificador_length;
		memcpy(&identificador_length, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		unaInstruccion->identificador = malloc(identificador_length);
		memcpy(unaInstruccion->identificador, buffer+desplazamiento, identificador_length);
		desplazamiento+=identificador_length;
		memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(pcb -> instrucciones, unaInstruccion);
		log_info(logger,"Instruccion: %s",unaInstruccion->identificador);
		i++;
	}
	memcpy(&pcb->program_counter, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	log_info(logger,"program counter: %d",pcb->program_counter);
	memcpy(&pcb->registros, buffer + desplazamiento, sizeof(t_registros));
	desplazamiento+=sizeof(t_registros);
	log_info(logger,"Registro AX: %d y DX: %d",pcb->registros.AX,pcb->registros.DX);
	memcpy(&contadorSegmentos, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	i=0;
	log_info(logger,"cont seg: %d",contadorSegmentos);
	while(i < contadorSegmentos){
		entradaTablaSegmento* unSegmento = malloc(sizeof(entradaTablaSegmento));
		memcpy(&(unSegmento->numeroSegmento),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&(unSegmento->numeroTablaPaginas),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&(unSegmento->tamanioSegmento),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		log_info(logger,"Nro: %d, numero tabla paginas %d, tamanio segmento %d",unSegmento->numeroSegmento,unSegmento->numeroTablaPaginas,unSegmento->tamanioSegmento);
		list_add(pcb->tabla_segmentos,unSegmento);
		i++;
	}
	memcpy(&pcb->socket, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(&(pcb->estado), buffer + desplazamiento, sizeof(t_estado));
	desplazamiento+=sizeof(t_estado);
	memcpy(&(pcb->algoritmoActual), buffer + desplazamiento, sizeof(t_algoritmo_pcb));
	desplazamiento+=sizeof(t_algoritmo_pcb);
	log_info(logger,"Estado: %d",pcb->estado);
	log_info(logger,"Algoritmo: %d",pcb->algoritmoActual);
	free(buffer);
	return pcb;
}

void obtenerTamanioIdentificadores(instruccion* unaInstruccion) {
	tamanioTotalIdentificadores += (strlen(unaInstruccion -> identificador)+1);
	contadorInstrucciones++;
}

void obtenerCantidadDeSegmentos(entradaTablaSegmento* unSegmento){
	contadorSegmentos++;
}

void agregarInstruccionesAlPaquete(instruccion* unaInstruccion) {
	char* id = unaInstruccion->identificador;
	int longitudId = strlen(unaInstruccion->identificador)+1;
	memcpy(paquete->buffer->stream + desplazamiento, &longitudId, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, id, longitudId);
	desplazamiento+=longitudId;
	memcpy(paquete->buffer->stream + desplazamiento, &(unaInstruccion->parametros), sizeof(int[2]));
	desplazamiento+=sizeof(int[2]);
	free(unaInstruccion->identificador);
	free(unaInstruccion);
}

void agregarSegmentosAlPaquete(entradaTablaSegmento* unSegmento){
	memcpy(paquete->buffer->stream + desplazamiento, &(unSegmento->numeroSegmento), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(unSegmento->numeroTablaPaginas), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(unSegmento->tamanioSegmento), sizeof(int));
	desplazamiento+=sizeof(int);
	free(unSegmento);
}
void enviar_pcb(t_pcb* pcb,int cod_op,int conexionCpu)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	contadorSegmentos = 0;
	desplazamiento = 0;
	paquete = crear_paquete(cod_op);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	list_iterate(pcb->tabla_segmentos, (void*) obtenerCantidadDeSegmentos);
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanioTotalIdentificadores +
										contadorInstrucciones*sizeof(int[2]) + contadorInstrucciones*sizeof(int) +
											3*sizeof(int)+ sizeof(t_registros) + contadorSegmentos*sizeof(entradaTablaSegmento) + sizeof(int)); // puede que falte un int
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->idProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorInstrucciones, sizeof(int));
	desplazamiento+=sizeof(int);
	list_iterate(pcb->instrucciones, (void*) agregarInstruccionesAlPaquete);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->program_counter), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->registros), sizeof(t_registros));
	desplazamiento+=sizeof(t_registros);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorSegmentos, sizeof(int));//ver tema contador
	desplazamiento+=sizeof(int);
	list_iterate(pcb->tabla_segmentos, (void*) agregarSegmentosAlPaquete);
	log_info(logger,"cont seg: %d",contadorSegmentos);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estado), sizeof(int));
	desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;

	enviar_paquete(paquete, conexionCpu);
	free(pcb);
	eliminar_paquete(paquete);
}


/*
void intento_de_enviar_Pcb(t_pcb * PCB,op_code codigo, int socketCliente){
	t_paquete * paqueteConPcb = crear_paquete(codigo);
	agregar_a_paquete(paqueteConPcb,PCB->idProceso,sizeof(int));

//	int tamanioInstrucciones = 0;
//	for(int i = 0;i<list_size(PCB->instrucciones);i++){
//			instruccion * instruccionAux = list_get(PCB->instrucciones,i);
	//		tamanioInstrucciones = tamanioInstrucciones + sizeof(instruccionAux->identificador) + sizeof(int[2]);
		//}

	int cantidadInstrucciones = list_size(PCB->instrucciones);
	agregar_a_paquete(paqueteConPcb, cantidadInstrucciones, sizeof(int));

	for(int i=0;i<list_size(PCB->instrucciones);i++){
		instruccion * instruccionAux = list_get(PCB->instrucciones,i);
		agregar_a_paquete_instrucciones(paqueteConPcb, instruccionAux, sizeof(instruccionAux->identificador));
	}

	agregar_a_paquete(paqueteConPcb,PCB->program_counter,sizeof(int));
	agregar_a_paquete(paqueteConPcb,PCB->registros.AX,sizeof(int));
	agregar_a_paquete(paqueteConPcb,PCB->registros.BX,sizeof(int));
	agregar_a_paquete(paqueteConPcb,PCB->registros.CX,sizeof(int));
	agregar_a_paquete(paqueteConPcb,PCB->registros.DX,sizeof(int));

	int cantidadSegmentos = list_size(PCB->tabla_segmentos);
		agregar_a_paquete(paqueteConPcb, cantidadSegmentos, sizeof(int));

	for(int i=0;i<list_size(PCB->tabla_segmentos);i++){
			t_tabla_segmentos * elemento = list_get(PCB->tabla_segmentos,i);
			agregar_a_paquete(paqueteConPcb, elemento->num_tabla_paginas, sizeof(int));
			agregar_a_paquete(paqueteConPcb, elemento->tam_segmento, sizeof(int));
		}

	int socket = PCB->socket;
	int estado = PCB->estado;
	int algoritmo = PCB->algoritmoActual;

	agregar_a_paquete(paqueteConPcb,socket,sizeof(int));
	agregar_a_paquete(paqueteConPcb,estado,sizeof(int));
	agregar_a_paquete(paqueteConPcb,algoritmo,sizeof(int));

	enviar_paquete(paqueteConPcb, socketCliente);
}
*/


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

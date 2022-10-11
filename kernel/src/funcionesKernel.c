#include "funcionesKernel.h"
/*
La idea seria tener las siguientes funciones:
- recibir instrucciones (LISTA DE T_INSTRUCCION)
- recibir tamanios de segmentos (LISTA DE INTS)
- enviar confirmacion de recepcion
- enviar pedido de impresion
- enviar valor a imprimir (INT)
- enviar pedido de valor por teclado
- recibir mensaje de desbloqueo teclado
- recibir valor de teclado (INT)
*/


// Hacemos uso de las siguientes funciones de servidor


t_log* logger;

int socketServidor;
t_list* listaInstrucciones;

char * ipMemoria;
char * puertoMemoria;
char * ipCpu;
char * puertoCpuDispatch;
char * puertoCpuInterrupt;
char* puertoKernel;
char * algoritmoPlanificacion;
int gradoMultiprogramacionTotal;
char** dispositivos_io;
char** tiempos_io;
int quantum_rr;
int identificadores_pcb;

t_queue* colaNew;
t_queue* colaReadyFIFO;
t_queue* colaReadyRR;
t_queue* colaBlockedPantalla;
t_queue* colaBlockedTeclado;
t_list* listaDeColasDispositivos;


sem_t kernelSinFinalizar;
sem_t gradoDeMultiprogramacion;
pthread_mutex_t mutexNew;

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

void recibir_mensaje(int socket_cliente)
{
int size;
char* buffer = recibir_buffer(&size, socket_cliente);
log_info(logger, "Me llego el mensaje %s", buffer);
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

void iterator(instruccion* instruccion){
	log_info(logger,"%s", instruccion->identificador);
}
/*
t_list * inicializar_tabla_segmentos(int socket_cliente) {
	t_list * listaSegmentos = list_create();
	listaSegmentos = recibir_lista_enteros(socket_cliente);
	t_list * tablaSegmentos = list_create();
	int i = 0;

	while (i<list_size(listaSegmentos)) {
		t_tabla_segmentos elemento;

		elemento.num_segmento = i;
		elemento.num_tabla_paginas = 0; // identificador de TP asociado
		elemento.tam_segmento = list_remove(listaSegmentos,0); // tamanio del segmento

		list_add(tablaSegmentos,elemento);

		i++;
	}
	return tablaSegmentos;
}
*/  //ver esto

void inicializar_registros(t_registros registros) {
	registros.AX = 0;
	registros.BX = 0;
	registros.CX = 0;
	registros.DX = 0;
}



//-------------------------HILOS---------------------------------

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,(void*) &dir_cliente, &tam_direccion);
	log_info(logger, "Se conecto la consola!");

	return socket_cliente;
}


int get_identificador(){
	identificadores_pcb++;
	return identificadores_pcb;
}



void recibir_consola(int servidor) {

	while (1) {
		pthread_t hilo1;
		log_info(logger,"esperamos una nueva consola");
		int nuevo_cliente = esperar_cliente(servidor);

		log_info(logger,"recibimos al cliente de socket %d", nuevo_cliente);

		int hiloCreado = pthread_create(&hilo1, NULL, &atender_consola,nuevo_cliente);
		pthread_detach(hilo1);

		log_info(logger,"levantamos y detacheamos el hilo de la consola %d", nuevo_cliente);
	}

}


void atender_consola(int nuevo_cliente) {
	t_pcb* PCB = malloc(sizeof(t_pcb));

	log_info(logger,"[atender_consola]recibimos las instrucciones del cliente %d!", nuevo_cliente);


	PCB->idProceso = get_identificador();
	PCB->tamanioProceso = sizeof(t_pcb) + 5 * sizeof(int) + sizeof(PCB->instrucciones) + sizeof(PCB->registros) + sizeof(PCB->tabla_segmentos) ;// consultar si lo hicimos bien
	PCB->instrucciones = list_create();
	PCB->instrucciones = recibir_lista_instrucciones(nuevo_cliente);
	PCB->program_counter = 0;
	inicializar_registros(PCB->registros);
	PCB->tabla_segmentos = list_create();
	//PCB->tabla_segmentos = inicializar_tabla_segmentos(nuevo_cliente); // aca usariamos el recibir_lista_enteros
	PCB->socket = nuevo_cliente;
	PCB->estado = NEW;

	log_info(logger,"[atender_consola]inicializamos PCB de id_proceso %d", PCB->idProceso);


	//pthread_mutex_lock(&encolandoPcb);
	//agregarANew(PCB);
	//pthread_mutex_unlock(&encolandoPcb);

	//log_info(logger,"[atender_consola] despertamos a asignar_memoria()");

	//sem_post(&pcbEnNew);
}





void agregarANew(t_pcb* proceso) {

	pthread_mutex_lock(&mutexNew);
	queue_push(colaNew, proceso);
	log_info(logger, "[NEW] Entra el proceso de PID: %d a la cola.",
			proceso->idProceso);
	log_info(logger,"el tamanio de la cola de new es %d", queue_size(colaNew));
	pthread_mutex_unlock(&mutexNew);
}

t_pcb* sacarDeNew() {

	pthread_mutex_lock(&mutexNew);
	t_pcb* proceso = queue_pop(colaNew);
	log_info(logger, "[NEW] Se saca el proceso de PID: %d de la cola",
			proceso->idProceso);
	pthread_mutex_unlock(&mutexNew);

	return proceso;
}



void asignar_memoria() {

	while (1) {

		//sem_wait(&pcbEnNew);
		//sem_wait(&gradoDeMultiprogramacion);
		t_pcb* proceso;
		log_info(logger, "[asignar_memoria] :se desperto ");

		//pthread_mutex_lock(&asignarMemoria);

		proceso = sacarDeNew();

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,proceso);
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,proceso);
			} else {
				log_info(logger, "Algoritmo invalido");
			}
		}
		//despues se agregaria el case para el multicolas que lo enviaria a la cola de RR
		//para que sea un switch habria que tener un conversor del string que leemos al enum de algoritmos que tenemos


		//pthread_mutex_unlock(&asignarMemoria);

		log_info(logger,"[asignar_memoria]: desde asignar_memoria despertamos a readyAExe");
	}

}

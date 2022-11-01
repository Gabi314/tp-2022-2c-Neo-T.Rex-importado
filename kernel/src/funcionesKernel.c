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

sem_t kernelSinFinalizar;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int contadorSegmentos;
int desplazamiento;


t_paquete* paquete;

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

void agregar_a_paquete_kernel_cpu(t_pcb* pcb,int cod_op,int conexionCpu)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	contadorSegmentos = 0;
	desplazamiento = 0;
	paquete = crear_paquete(cod_op);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	list_iterate(pcb->tablaSegmentos, (void*) obtenerCantidadDeSegmentos);
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanioTotalIdentificadores +
										contadorInstrucciones*sizeof(int[2]) + contadorInstrucciones*sizeof(int) +
											3*sizeof(int)+ sizeof(t_registros) + contadorSegmentos*sizeof(entradaTablaSegmento) + sizeof(int));
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->idProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorInstrucciones, sizeof(int));
	desplazamiento+=sizeof(int);
	list_iterate(pcb->instrucciones, (void*) agregarInstruccionesAlPaquete);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->programCounter), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->registros), sizeof(t_registros));
	desplazamiento+=sizeof(t_registros);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorSegmentos, sizeof(int));//ver tema contador
	desplazamiento+=sizeof(int);
	list_iterate(pcb->tablaSegmentos, (void*) agregarSegmentosAlPaquete);
	log_info(logger,"cont seg: %d",contadorSegmentos);
//	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket_cliente), sizeof(int));
//	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estado), sizeof(int));
	desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;

	enviar_paquete(paquete, conexionCpu);
	free(pcb);
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

void iterator(instruccion* instruccion){
	log_info(logger,"%s", instruccion->identificador);
}

t_list * inicializar_tabla_segmentos(int socket_cliente) {
	t_list * listaSegmentos = list_create();
	listaSegmentos = recibir_lista_enteros(socket_cliente);
	t_list * tablaSegmentos = list_create();
	int i = 0;

	while (i<list_size(listaSegmentos)) {
		int elemento[2];
		elemento[0] = 0; // identificador de TP asociado
		elemento[1] = list_remove(listaSegmentos,0); // tamanio del segmento

		list_add(tablaSegmentos,elemento);
		i++;
	}
	return tablaSegmentos;
}

void inicializar_registros(int v[4]) {
	int i=0;
	while (i<4){
		v[i] = 0;
		i++;
	}

}

//-------------------------HILOS---------------------------------


/*
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
*/
/*
void atender_consola(int nuevo_cliente) {
	t_pcb* PCB = malloc(sizeof(t_pcb));

	log_info(logger,"[atender_consola]recibimos las instrucciones del cliente %d!", nuevo_cliente);


	PCB->idProceso = 0; //poner un contador
	PCB->tamanioProceso = 1; // inicializar bien en caso de usarlo
	PCB->instrucciones = list_create();
	PCB->instrucciones = recibir_lista_instrucciones(nuevo_cliente);
	PCB->program_counter = 0;
	inicializar_registros(PCB->registros);
	PCB->tabla_segmentos = list_create();
	PCB->tabla_segmentos = inicializar_tabla_segmentos(nuevo_cliente); // aca usariamos el recibir_lista_enteros
	PCB->socket = nuevo_cliente;
	PCB->estado = NEW;

	log_info(logger,"[atender_consola]inicializamos PCB de id_proceso %d", PCB->idProceso);


	//pthread_mutex_lock(&encolandoPcb);
//	agregarANew(PCB);
//	pthread_mutex_unlock(&encolandoPcb);

//	log_info(logger,"[atender_consola] despertamos a asignar_memoria()");

//	sem_post(&pcbEnNew);
}
*/


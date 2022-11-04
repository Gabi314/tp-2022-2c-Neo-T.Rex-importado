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

t_pcb* pcbTeclado;
int registroTeclado;
t_pcb* pcbPantalla;
int registroPantalla;


t_queue* colaNew;
t_queue* colaReadyFIFO;
t_queue* colaReadyRR;
t_queue* colaBlockedPantalla;
t_queue* colaBlockedTeclado;
t_list* listaDeColasDispositivos;

sem_t cpuDisponible;
sem_t pcbEnNew;
sem_t pcbEnReady;
sem_t kernelSinFinalizar;
sem_t gradoDeMultiprogramacion;
sem_t pcbEnReady;
sem_t pcbEnNew;
sem_t cpuDisponible;
pthread_mutex_t mutexNew;
pthread_mutex_t obtenerProceso;
pthread_mutex_t mutexPantalla;
pthread_mutex_t mutexTeclado;



void iterator(instruccion* instruccion){
	log_info(logger,"%s param1: %d param2: %d", instruccion->identificador, instruccion->parametros[0],instruccion->parametros[1]);
}

t_list * inicializar_tabla_segmentos(int socket_cliente) {
	t_list * listaSegmentos = list_create();
	listaSegmentos = recibir_lista_enteros(socket_cliente);
	int tamanioDelSegmento = (int) list_get(listaSegmentos,3);
			log_info(logger,"El tamanio del primer segmento es: %d",tamanioDelSegmento);
	t_list * tablaSegmentos = list_create();
	int i = 0;

	while (i<list_size(listaSegmentos)) {
		t_tabla_segmentos * elemento;

		elemento->num_tabla_paginas = 0; // identificador de TP asociado
		elemento->tam_segmento = list_remove(listaSegmentos,0); // tamanio del segmento

		list_add(tablaSegmentos,elemento);

		i++;
	}
	return tablaSegmentos;
}


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

	PCB->instrucciones = list_create();
	PCB->instrucciones = recibir_lista_instrucciones(nuevo_cliente);
	PCB->program_counter = 0;
	inicializar_registros(PCB->registros);
	PCB->tabla_segmentos = list_create();
	PCB->tabla_segmentos = inicializar_tabla_segmentos(nuevo_cliente); // aca usariamos el recibir_lista_enteros
	PCB->socket = nuevo_cliente;
	PCB->estado = NEW;

	log_info(logger,"[atender_consola]inicializamos PCB de id_proceso %d", PCB->idProceso);

	agregarANew(PCB);


	//log_info(logger,"[atender_consola] despertamos a asignar_memoria()");

	sem_post(&pcbEnNew);
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

		sem_wait(&pcbEnNew);
		sem_wait(&gradoDeMultiprogramacion);
		t_pcb* proceso;
		log_info(logger, "[asignar_memoria] :se desperto ");

		//pthread_mutex_lock(&asignarMemoria); VER SI ESTE MUTEX ES NECESARIO

		proceso = sacarDeNew();

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,proceso);
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,proceso);
			} else {
				if(!strcmp(algoritmoPlanificacion, "MULTICOLAS")) {// ver si esta asi en los config
					queue_push(colaReadyRR,proceso);
				}else{
					log_info(logger, "Algoritmo invalido");
				}
			}
		}

		sem_post(&pcbEnReady);

		//para que sea un switch habria que tener un conversor del string que leemos al enum de algoritmos que tenemos

		//pthread_mutex_unlock(&asignarMemoria);

		log_info(logger,"[asignar_memoria]: desde asignar_memoria despertamos a readyAExe");
	}

}


void readyAExe() {

	while (1) {

		sem_wait(&pcbEnReady);
		sem_wait(&cpuDisponible);

	//	log_info(logger, "[readyAExe]: .........se despierta readyAExe ");
	//	log_info(logger, "[readyAExe]: a partir de ahora, CPU NO DISPONIBLE! ");

		pthread_mutex_lock(&obtenerProceso);
		t_pcb * procesoAEjecutar = obtenerSiguienteDeReady();
		pthread_mutex_unlock(&obtenerProceso);

		log_info(logger,
			"[readyAExe]: Desde readyAExe mandamos al proceso de pid %d a ejecutar  ",
			procesoAEjecutar->idProceso);

		ejecutar(procesoAEjecutar);

	//	log_info(logger,"[readyAExe]: finaliza");
	}
}


t_pcb* obtenerSiguienteDeReady() {

t_pcb * procesoPlanificado;

if (!strcmp(algoritmoPlanificacion, "FIFO")) {
	procesoPlanificado = obtenerSiguienteFIFO();
} else {
	if (!strcmp(algoritmoPlanificacion, "RR")) {
		procesoPlanificado = obtenerSiguienteRR();
	} else {
		log_info(logger, "[obtenerSiguienteDeReady]: algoritmo invalido");
		}
	}
	return procesoPlanificado;
}


t_pcb* obtenerSiguienteFIFO() {

	t_pcb* procesoPlanificado = queue_pop(colaReadyFIFO);

	log_info(logger, "[obtenerSiguienteFIFO]: PROCESOS EN READY FIFO: %d \n", queue_size(colaReadyFIFO));

	return procesoPlanificado;
}

t_pcb* obtenerSiguienteRR() {

	t_pcb* procesoPlanificado = queue_pop(colaReadyRR);

	log_info(logger, "[obtenerSiguienteFIFO]: PROCESOS EN READY RR: %d \n", queue_size(colaReadyRR));

	return procesoPlanificado;
}



void ejecutar(t_pcb* proceso) {

	if (proceso != NULL) {

		log_info(logger, "[EXEC] Ingresa el proceso de PID: %d",proceso->idProceso);

	} else {
		log_info(logger, "[EXEC] No se logró encontrar un proceso para ejecutar");
	}

 //	conexionConCpu(proceso); Aca mandamos el proceso a cpu

	log_info(logger, "[ejecutar]: enviamos el proceso a cpu");

}

void atender_interrupcion_de_ejecucion() {

while (1) {
	int cod_op = recibir_operacion(puertoCpuDispatch); //Puerto o socket

	switch (cod_op) {

	case IO_GENERAL:
		break;
	case IO_TECLADO:
		/*
		t_info_teclado aMandar;
		aMandar.pcb = recibir_pcb(puertoCpuDispatch);
		recibir_operacion(puertoCpuDispatch);
		aMandar.registro =recibir_entero(puertoCpuDispatch);
		*/
		pthread_mutex_lock(&mutexTeclado);
		pcbTeclado = recibir_pcb(puertoCpuDispatch);
		recibir_operacion(puertoCpuDispatch);
		registroTeclado =recibir_entero(puertoCpuDispatch);
		pthread_mutex_unlock(&mutexTeclado);
		pthread_t hiloAtenderTeclado;
		int hiloTeclado = pthread_create(&hiloAtenderTeclado, NULL,&atender_IO_teclado,NULL);
		pthread_detach(hiloTeclado);
		break;
	case IO_PANTALLA:
		pthread_mutex_lock(&mutexPantalla);
		pcbPantalla = recibir_pcb(puertoCpuDispatch);
		recibir_operacion(pcbPantalla->socket);
		registroPantalla = recibir_entero(puertoCpuDispatch);
		pthread_mutex_unlock(&mutexPantalla);
		pthread_t hiloPantalla;
		int hiloPantallaCreado = pthread_create(&hiloPantalla, NULL, &atender_IO_pantalla, NULL);
		pthread_detach(hiloPantallaCreado);
		break;
	case QUANTUM:
		t_pcb* pcb = recibir_pcb(puertoCpuDispatch);

		if (!strcmp(algoritmoPlanificacion, "RR")) {
			queue_push(colaReadyRR,pcb);
		} else {
			if (!strcmp(algoritmoPlanificacion, "Feedback")) {
				queue_push(colaReadyFIFO,pcb);
			} else {
					log_info(logger, "Algoritmo invalido");
			}
		}
		sem_post(&pcbEnReady);

		break;
	case TERMINAR_PROCESO:
		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos operacion EXIT");
		terminarEjecucion(recibir_pcb(puertoCpuDispatch));
		break;
	default:
		log_info(logger, "operacion invalida");
		break;
	}

	//ejecucionActiva = false;
	sem_post(&cpuDisponible);

	}
}

void terminarEjecucion(t_pcb * procesoAFinalizar) {


	procesoAFinalizar->estado = TERMINATED; // es local no hace falta mutex

	//enviar_mensaje("Liberar estructuras",socketMemoria,MENSAJE_LIBERAR_POR_TERMINADO);//Enviamos mensaje a memoria para que libere estructuras

	log_info(logger, "[terminarEjecucion]: Se envia un mensaje a memoria para que libere estructuras");

	//	enviar_mensaje("Finalizo la ejecucion",unaConsola->socket,MENSAJE_FINALIZAR_EXE); //Avisamos a consola que finalizo el proceso
	//	liberar_conexion(unaConsola->socket);

	log_info(logger, "[terminarEjecucion]: Aumenta grado de multiprogramacion actual y termina terminarEjecucion!!");

	sem_post(&gradoDeMultiprogramacion);

	destruirProceso(procesoAFinalizar);

}

void destruirProceso(t_pcb* proceso) {
	close(proceso->socket);
	free(proceso);
}

void atender_IO_teclado(){

		//t_pcb* unPcb = info.pcb;

		enviar_mensaje("kernel solicita que se ingrese un valor por teclado",pcbTeclado->socket,KERNEL_MENSAJE_SOLICITUD_VALOR_POR_TECLADO);

		int codigo = recibir_operacion(pcbTeclado->socket);
			if(codigo != KERNEL_MENSAJE_DESBLOQUEO_TECLADO){
					log_info(logger,"codigo de operacion incorrecto");
				}
		recibir_mensaje(pcbTeclado->socket);

		codigo = recibir_operacion(pcbTeclado->socket);

		if(codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO){
						log_info(logger,"codigo de operacion incorrecto");
					}
		int enteroRecibido = recibir_entero(pcbTeclado->socket);

		switch(registroTeclado){
			case 0:
				(&pcbTeclado->registros)->AX = enteroRecibido;
				break;
			case 1:
				(&pcbTeclado->registros)->BX = enteroRecibido;
				break;
			case 2:
				(&pcbTeclado->registros)->CX = enteroRecibido;
				break;
			case 3:
				(&pcbTeclado->registros)->DX = enteroRecibido;
				break;
		}

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,pcbTeclado);
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,pcbTeclado);
			} else {
				if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
					queue_push(colaReadyRR,pcbTeclado);
				}else{
					log_info(logger, "Algoritmo invalido");
				}
			}
		}

		sem_post(&pcbEnReady);
}

void atender_IO_pantalla() {
	int valor_registro = buscar_valor_registro(pcbPantalla,registroPantalla);

	enviar_entero(valor_registro, pcbPantalla->socket, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);
	recibir_operacion(pcbPantalla->socket);
	recibir_mensaje(pcbPantalla->socket);
}

int buscar_valor_registro(t_pcb* pcb, int registro) {
	switch (registro) {
	case 0:
		return (&pcb->registros)->AX;
	case 1:
		return (&pcb->registros)->BX;
	case 2:
		return (&pcb->registros)->CX;
	case 3:
		return (&pcb->registros)->DX;
	default:
		return -1;
	}
}


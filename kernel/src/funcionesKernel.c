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

pthread_t hiloQuantumRR;


sem_t kernelSinFinalizar;
sem_t gradoDeMultiprogramacion;
sem_t pcbEnReady;
sem_t pcbEnNew;
sem_t cpuDisponible;
pthread_mutex_t mutexNew;
pthread_mutex_t obtenerProceso;
pthread_mutex_t mutexPantalla;
pthread_mutex_t mutexTeclado;


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
		entradaTablaSegmento * elemento;

		elemento->numeroTablaPaginas = 0; // identificador de TP asociado
		elemento->tamanioSegmento = list_remove(listaSegmentos,0); // tamanio del segmento

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



void levantar_hilo_dispositivo(t_elem_disp * elemento) {
	pthread_t hiloDispositivo;
	int hiloDispositivoCreado = pthread_create(&hiloDispositivo,NULL,(void*) atender_IO_generico,elemento);
	pthread_detach(hiloDispositivo);

}





//-------------------------HILOS---------------------------------


/*
void recibir_consola(int servidor) {
=======
	socklen_t tam_direccion = sizeof(struct sockaddr_in);
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,(void*) &dir_cliente, &tam_direccion);
	log_info(logger, "Se conecto la consola!");

	return socket_cliente;
}

*/
int get_identificador(){
	identificadores_pcb++;
	return identificadores_pcb;
}



void recibir_consola(int * servidor) {

	int servidor_int = *servidor;


	while (1) {
		pthread_t hilo1;
		log_info(logger,"esperamos una nueva consola");
		int nuevo_cliente = esperar_cliente(servidor_int,"Consola");

		log_info(logger,"recibimos al cliente de socket %d", nuevo_cliente);

		int hiloCreado = pthread_create(&hilo1, NULL,  (void *)atender_consola, &nuevo_cliente);
		pthread_detach(hilo1);

		log_info(logger,"levantamos y detacheamos el hilo de la consola %d", nuevo_cliente);
	}

}


void  atender_consola(int * nuevo_cliente) {
	t_pcb* PCB = malloc(sizeof(t_pcb));
	int  nuevo_cliente_int = *nuevo_cliente;

	log_info(logger,"[atender_consola]recibimos las instrucciones del cliente %d!", nuevo_cliente_int);


	PCB->idProceso = get_identificador();

	PCB->instrucciones = list_create();
	PCB->instrucciones = recibir_paquete_instrucciones(nuevo_cliente_int);
	PCB->programCounter = 0;
	inicializar_registros(PCB->registros);
	PCB->tablaSegmentos = list_create();
	PCB->tablaSegmentos = inicializar_tabla_segmentos(nuevo_cliente_int); // aca usariamos el recibir_lista_enteros
	PCB->socket = nuevo_cliente_int;
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
				if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
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

		if(algoritmoPlanificacion == "RR" || (algoritmoPlanificacion == "Feedback" && procesoAEjecutar->algoritmoActual == RR)){
			int hiloQuantum = pthread_create(&hiloQuantumRR, NULL,(void*) controlar_quantum,NULL);
			pthread_detach(hiloQuantum);
		}

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
			if(!strcmp(algoritmoPlanificacion, "Feedback")){
				if(queue_size(colaReadyRR)>0){
					procesoPlanificado = obtenerSiguienteRR();
					procesoPlanificado->algoritmoActual = RR;
				}else{
					procesoPlanificado = obtenerSiguienteFIFO();
					procesoPlanificado->algoritmoActual = FIFO;
				}
			}else{

				log_info(logger, "[obtenerSiguienteDeReady]: algoritmo invalido");
			}
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
 //	agregarAPaqueteKernelCpu(proceso); ?


	log_info(logger, "[ejecutar]: enviamos el proceso a cpu");

}



/*void atender_interrupcion_de_ejecucion() {

while (1) {
	int cod_op = recibir_operacion(puertoCpuDispatch); //Puerto o socket (Cuando iniciemos el servidor en el main esto cambia)

	switch (cod_op) {

	case IO_GENERAL:
		t_pcb * proceso = recibir_pcb(puertoCpuDispatch);

		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion, "Feedback") && proceso->algoritmoActual == RR)){
			pthread_cancel(hiloQuantumRR);
			}

		char * dispositivo;

		recibir_operacion(puertoCpuDispatch);
		dispositivo = recibir_cadena(puertoCpuDispatch);

		bool comparar_nombre_dispositivo( t_elem_disp * elemento){
			return !strcmp(elemento->dispositivo,dispositivo);
		}

		t_elem_disp * elementoLista = list_find(listaDeColasDispositivos, comparar_nombre_dispositivo);

		queue_push(elementoLista->cola_procesos,proceso);

		recibir_operacion(puertoCpuDispatch);
		int unidadesTrabajo = recibir_entero(puertoCpuDispatch);
		queue_push(elementoLista->cola_UTs,&unidadesTrabajo);

		sem_post(&elementoLista->semaforo);


		break;
	case IO_TECLADO:


		t_info_teclado * aMandarTeclado = malloc(sizeof(*aMandarTeclado));
		aMandarTeclado->pcb = recibir_pcb(puertoCpuDispatch);


		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion, "Feedback") && aMandarTeclado->pcb->algoritmoActual == RR)){
									pthread_cancel(hiloQuantumRR);
								}

		recibir_operacion(puertoCpuDispatch);
		aMandarTeclado->registro =recibir_entero(puertoCpuDispatch);




		//pthread_mutex_lock(&mutexTeclado);
		t_pcb * pcbTeclado = recibir_pcb(puertoCpuDispatch); // la idea es que sea una varible loca
		recibir_operacion(puertoCpuDispatch);
		registroTeclado =recibir_entero(puertoCpuDispatch);
		//pthread_mutex_unlock(&mutexTeclado);


		pthread_t hiloAtenderTeclado;
		int hiloTeclado = pthread_create(&hiloAtenderTeclado, NULL,(void*) atender_IO_teclado,aMandarTeclado);
		pthread_detach(hiloAtenderTeclado);

		//free(aMandar.pcb);
		//free(aMandar.registro);
		//free(aMandar);

		break;

	case IO_PANTALLA:



		t_info_pantalla * aMandarPantalla = malloc(sizeof(*aMandarPantalla));

	//	pthread_mutex_lock(&mutexPantalla);
		aMandarPantalla->pcb = recibir_pcb(puertoCpuDispatch);

		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion,"Feedback") && aMandarPantalla->pcb->algoritmoActual == RR)){
						pthread_cancel(hiloQuantumRR);
					}

		recibir_operacion(puertoCpuDispatch);
		registroPantalla = recibir_entero(puertoCpuDispatch);
	//	pthread_mutex_unlock(&mutexPantalla);


		pthread_t hiloPantalla;
		int hiloPantallaCreado = pthread_create(&hiloPantalla, NULL, &atender_IO_pantalla, aMandarPantalla);
		pthread_detach(hiloPantalla);

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

	case PAGE_FAULT:

		t_pcb* pcbPF = recibir_pcb(puertoCpuDispatch);

		//recibir pagina a cargar desde cpu

		pthread_t hiloPF;
		int hiloPageFault = pthread_create(&hiloPF, NULL, &atender_page_fault, pcbPF);
		pthread_detach(hiloPageFault);

		break;

	case TERMINAR_PROCESO:
		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos operacion EXIT");

		t_pcb* pcbATerminar = recibir_pcb(puertoCpuDispatch);

		if(algoritmoPlanificacion == "RR" || (algoritmoPlanificacion == "Feedback" && pcbATerminar->algoritmoActual == RR)){
			pthread_cancel(hiloQuantumRR);
		}

		terminarEjecucion(pcbATerminar);
		break;
	default:
		log_info(logger, "operacion invalida");
		break;
	}

	//ejecucionActiva = false;
	sem_post(&cpuDisponible);

	}
}

*/

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

void atender_IO_teclado(t_info_teclado * info){

		t_pcb* unPcb = info->pcb;

		enviar_mensaje("kernel solicita que se ingrese un valor por teclado",unPcb->socket,KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO);

		int codigo = recibir_operacion(unPcb->socket);
			if(codigo != KERNEL_MENSAJE_DESBLOQUEO_TECLADO){
					log_info(logger,"codigo de operacion incorrecto");
				}
		recibir_mensaje(unPcb->socket);

		codigo = recibir_operacion(unPcb->socket);

		if(codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO){
						log_info(logger,"codigo de operacion incorrecto");
					}
		int enteroRecibido = recibir_entero(unPcb->socket);

		switch(info->registro){
			case 0:
				(&unPcb->registros)->AX = enteroRecibido;
				break;
			case 1:
				(&unPcb->registros)->BX = enteroRecibido;
				break;
			case 2:
				(&unPcb->registros)->CX = enteroRecibido;
				break;
			case 3:
				(&unPcb->registros)->DX = enteroRecibido;
				break;
		}

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,unPcb);
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,unPcb);
			} else {
				if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
					queue_push(colaReadyRR,unPcb);
				}else{
					log_info(logger, "Algoritmo invalido");
				}
			}
		}

		sem_post(&pcbEnReady);
}

void atender_IO_pantalla(t_info_pantalla * info) {
	t_pcb * unPcb = info->pcb;

	int valor_registro = buscar_valor_registro(unPcb ,info->registro);

	enviar_entero(valor_registro, unPcb->socket, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

	recibir_operacion(unPcb->socket);

	recibir_mensaje(unPcb->socket);

	if (!strcmp(algoritmoPlanificacion, "FIFO")) {
				queue_push(colaReadyFIFO,unPcb);
			} else {
				if (!strcmp(algoritmoPlanificacion, "RR")) {
					queue_push(colaReadyRR,unPcb);
				} else {
					if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
						queue_push(colaReadyRR,unPcb);
					}else{
						log_info(logger, "Algoritmo invalido");
					}
				}
			}

	sem_post(&pcbEnReady);
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

void controlar_quantum(){

	usleep(quantum_rr * 1000);

	//enviar_mensaje("Cpu desalojá tu proceso por fin de quantum",  ,DESALOJAR_PROCESO_POR_FIN_DE_QUANTUM); // ES EL SOCKET

}

void atender_IO_generico(t_elem_disp* elemento){
	while(1) {
		sem_wait(&elemento->semaforo);

		int * unidades_trabajo = queue_pop(elemento->cola_UTs);
		int tiempo_dispositivo = elemento->tiempo;

		usleep(tiempo_dispositivo * 1000 * (*unidades_trabajo));

		t_pcb * proceso = queue_pop(elemento->cola_procesos);

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
						queue_push(colaReadyFIFO,proceso);
					} else {
						if (!strcmp(algoritmoPlanificacion, "RR")) {
							queue_push(colaReadyRR,proceso);
						} else {
							if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
								queue_push(colaReadyRR,proceso);
							}else{
								log_info(logger, "Algoritmo invalido");
							}
						}
					}
		sem_post(&pcbEnReady);
	}


}



void atender_page_fault(t_pcb* pcb){
	pcb->estado = BLOCKED; // hace falta un nuevo estado BLOCKED_PF?

//	enviar_mensaje("kernel solicita que se cargue en memoria la pagina correspondiente",socket que corresponda, KERNEL_MENSAJE_SOLICITUD_CARGAR_PAGINA);

	//hay que mandar la pagina recibida de cpu

//	int codigo = recibir_operacion(socket que corresponda);

//	if(codigo != KERNEL_MENSAJE_CONFIRMACION_PF){
//		log_info(logger,"codigo de operacion incorrecto");
//	}
//
//	recibir_mensaje(socket que corresponda);

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


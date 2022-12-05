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
t_log* loggerAux;

int socketServidorKernel;
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
int conexionCpuInterrupt;
int conexionCpuDispatch;
int socketMemoria;
char* dispositivosIOAplanado;


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
pthread_mutex_t primerPushColaReady;
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

void enviar_pcb(t_pcb* pcb,int cod_op,int conexionCpu)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	contadorSegmentos = 0;
	desplazamiento = 0;
	paquete = crear_paquete(cod_op);

	//controlar_pcb(pcb);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	list_iterate(pcb->tablaSegmentos, (void*) obtenerCantidadDeSegmentos);
	paquete->buffer->stream = realloc(paquete->buffer->stream,
			paquete->buffer->size + tamanioTotalIdentificadores
					+ contadorInstrucciones * sizeof(int[2])
					+ contadorInstrucciones * sizeof(int)
					+ 5 * sizeof(int)
					+ sizeof(t_registros)
					+ contadorSegmentos * sizeof(entradaTablaSegmento)
					+ sizeof(t_estado)
					+ sizeof(t_algoritmo_pcb));
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

	//log_info(loggerAux,"cont seg: %d",contadorSegmentos);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estado), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->algoritmoActual), sizeof(int));
	desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;

	enviar_paquete(paquete, conexionCpu);
	eliminar_paquete(paquete);
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



/*
void iterator(instruccion* instruccion){
	log_info(loggerAux,"%s param1: %d param2: %d", instruccion->identificador, instruccion->parametros[0],instruccion->parametros[1]);
}
*/


t_list * inicializar_tabla_segmentos(int socket_cliente,int codigo) {
	t_list * listaSegmentos = list_create();
	//recibir con cod_op
	//poner if igualando cod_op con el codigo de consola

	//int codigo = recibir_operacion(socket_cliente);
	if(codigo == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS){
		listaSegmentos = recibir_lista_enteros(socket_cliente);
	}else{
		log_info(logger,"No se recibio la lista correctamente");
	}
	log_info(logger,"tamanios de segmentos recibidos");
	int tamanioDelSegmento = (int) list_get(listaSegmentos,0);
	log_info(loggerAux,"El tamanio del primer segmento es: %d",tamanioDelSegmento);

	t_list * tablaSegmentos = list_create();
	int i = 0;
	int tamanioLista = list_size(listaSegmentos);
	while (i<tamanioLista) {
		entradaTablaSegmento * elemento = malloc(sizeof(entradaTablaSegmento));

		elemento->numeroSegmento = i;
		elemento->numeroTablaPaginas = 0; // identificador de TP asociado, esto viene de memoria

		int  tamanio =  (int) list_remove(listaSegmentos,0);
		log_info(logger,"el tamanio es %d",tamanio);
		elemento->tamanioSegmento = tamanio ;// tamanio del segmento


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
	pthread_create(&hiloDispositivo,NULL,(void*) atender_IO_generico,elemento);
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
	log_info(logger,"se desperto recibir_consola");

	while (1) {
		pthread_t hilo1;

		log_info(loggerAux,"esperamos una nueva consola");

		int nuevo_cliente = esperar_cliente(servidor_int,"Consola");

		log_info(loggerAux,"recibimos al cliente de socket %d", nuevo_cliente);

		pthread_create(&hilo1, NULL,  (void *)atender_consola, &nuevo_cliente);
		pthread_detach(hilo1);

		log_info(loggerAux,"levantamos y detacheamos el hilo de la consola %d", nuevo_cliente);
	}

}


void  atender_consola(int * nuevo_cliente) {
	t_pcb* PCB = malloc(sizeof(t_pcb));
	int  nuevo_cliente_int = *nuevo_cliente;


	log_info(loggerAux,"[atender_consola]recibimos las instrucciones y segmentos del cliente %d!", nuevo_cliente_int);



	log_info(logger,"[atender_consola]esperando segmentos");
		int operacion = recibir_operacion(nuevo_cliente_int);



		if (operacion == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {

			PCB->tablaSegmentos = list_create();
			PCB->tablaSegmentos = inicializar_tabla_segmentos(nuevo_cliente_int,operacion); // aca usariamos el recibir_lista_enteros


	//SE ENVIAN LOS DISPOSITIVOS
			log_info(logger,"[atender_consola]enviamos los dispositivos");
			enviar_mensaje(dispositivosIOAplanado, nuevo_cliente_int,
					KERNEL_MENSAJE_DISPOSITIVOS_IO);

	//SE RECIBEN LAS INSTRUCCIONES
			log_info(logger,"[atender_consola]esperando instrucciones");
			operacion =	recibir_operacion(nuevo_cliente_int);
			if ( operacion == KERNEL_PAQUETE_INSTRUCCIONES) {
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete_instrucciones(nuevo_cliente_int);

	//MOSTRAMOS LAS INSTRUCCIONES
				log_info(logger, "[atender_consola]me llegaron las instrucciones");
				list_iterate(listaInstrucciones, (void*) iteratorMostrarInstrucciones);

	//METEMOS LAS INSTRUCCIONES EN EL PCB

				PCB->instrucciones = list_create();
				PCB->instrucciones = listaInstrucciones;
				log_info(logger,"[atender_consola]metemos instrucciones en el pcb");

	// CONFIRMAMOS RECEPCION DE INSTRUCCIONES
				enviar_mensaje("[atender_consola]segmentos e instrucciones recibidos pibe", nuevo_cliente_int,KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS);

			} else {
				log_info(logger, "[atender_consola]codigo de operacion incorrecto %d", operacion);
			}
		}else{
			log_info(logger, "[atender_consola]codigo de operacion incorrecto %d", operacion);
		}

	PCB->idProceso = get_identificador();
	PCB->programCounter = 0;
	inicializar_registros(PCB->registros);
	PCB->socket = nuevo_cliente_int;
	PCB->estado = NEW;
	PCB->algoritmoActual = RR;

	log_info(loggerAux,"[atender_consola]inicializamos PCB de id_proceso %d", PCB->idProceso);

	log_info(logger,"Se crea el proceso <%d> en NEW", PCB->idProceso);

	agregarANew(PCB);


	log_info(logger,"[atender_consola] despertamos a asignar_memoria()");

	sem_post(&pcbEnNew);
}




void agregarANew(t_pcb* proceso) {

	pthread_mutex_lock(&mutexNew);
	queue_push(colaNew, proceso);
	log_info(loggerAux, "[NEW] Entra el proceso de PID: %d a la cola.",
			proceso->idProceso);
	log_info(loggerAux,"el tamanio de la cola de new es %d", queue_size(colaNew));
	pthread_mutex_unlock(&mutexNew);
}

t_pcb* sacarDeNew() {


	//pthread_mutex_lock(&mutexNew);
	t_pcb* proceso = malloc(sizeof(t_pcb));// = malloc(sizeof(t_pcb*));
	proceso = queue_pop(colaNew);
	log_info(loggerAux, "[NEW] Se saca el proceso de PID: %d de la cola",

			proceso->idProceso);
	log_info(logger,"el tamanio de la cola de new es %d", queue_size(colaNew));
//	pthread_mutex_unlock(&mutexNew);

	return proceso;
}



void asignar_memoria() {
	log_info(logger,"se desperto asignar_memoria por primera vez");
	while (1) {

		sem_wait(&pcbEnNew);
		sem_wait(&gradoDeMultiprogramacion);

		t_pcb* proceso = malloc(sizeof(t_pcb));
		log_info(loggerAux, "[asignar_memoria] :se desperto ");

		proceso = sacarDeNew();
		t_pcb* pcbAux= malloc(sizeof(t_pcb)); // malloc?

		log_info(loggerAux,"llego antes de crear el paquete");
		t_paquete * paqueteLocal = crear_paquete(NRO_TP);
		log_info(loggerAux,"llego antes de agregar dos enteros al paquete");
		agregar_a_paquete_unInt(paqueteLocal, &(proceso->idProceso), sizeof(int));
		int tamanioTablaSegmentos = list_size(proceso->tablaSegmentos);
		agregar_a_paquete_unInt(paqueteLocal, &tamanioTablaSegmentos, sizeof(int));
		log_info(loggerAux,"llego antes de enviar el paquete");
		enviar_paquete(paqueteLocal,socketMemoria);

		int cod_op = recibir_operacion(socketMemoria);
		int numTablaPag = 0;

		if(cod_op == MEMORIA_A_KERNEL_NUMERO_TABLA_PAGINAS) {
			numTablaPag = recibir_entero(socketMemoria);
			log_info(loggerAux,"Es el nro de tp correspondiente al proceso actual");
		}
		else {
			log_info(loggerAux,"operacion de memoria invalida");
		}

		log_info(loggerAux,"llego antes del for");
		for(int i=0;i<list_size(proceso->tablaSegmentos);i++ ){
					entradaTablaSegmento * entrada = malloc(sizeof(entradaTablaSegmento));
					entrada = list_get(proceso->tablaSegmentos,i); //ver que devuelve
					entrada->numeroTablaPaginas = numTablaPag+i;
					list_replace(proceso->tablaSegmentos,i,entrada);
					//log_info(loggerAux, list_get());
				}

		log_info(loggerAux,"llego antes del push a cola de ready");
			pthread_mutex_lock(&primerPushColaReady);
		

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,proceso);

			log_info(logger, "Cola Ready <FIFO>: [<LISTA DE PIDS>]");
			for(int i=0; i<queue_size(colaReadyFIFO); i++){
				pcbAux = list_get(colaReadyFIFO->elements,i);
				log_info(logger, "%d", pcbAux->idProceso);
			}
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,proceso);

				log_info(logger, "Cola Ready <RR>: [<LISTA DE PIDS>]");
				for(int i=0; i<queue_size(colaReadyRR); i++){
					pcbAux = list_get(colaReadyRR->elements,i);
					log_info(logger, "%d", pcbAux->idProceso);
				}
			} else {
				if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
					queue_push(colaReadyRR,proceso);

					log_info(logger, "Cola Ready <Feedback>: [<LISTA DE PIDS RR>]");
					for(int i=0; i<queue_size(colaReadyRR); i++){
						pcbAux = list_get(colaReadyRR->elements,i);
						log_info(logger, "%d", pcbAux->idProceso);
					}
					log_info(logger,"[<LISTA DE PIDS FIFO>]");
					for(int i=0; i<queue_size(colaReadyFIFO); i++){
						pcbAux = list_get(colaReadyFIFO->elements,i);
						log_info(logger, "%d", pcbAux->idProceso);
					}
				}else{
					log_info(logger, "Algoritmo invalido");
				}
			}
		}
		//free(pcbAux);
		log_info(logger,"PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <READY>", proceso->idProceso);
		sem_post(&pcbEnReady);
		log_info(loggerAux,"[asignar_memoria]: desde asignar_memoria despertamos a readyAExe");
	}
}


void readyAExe() {
	log_info(logger,"se desperto readyAExe por primera vez");
	while (1) {

		sem_wait(&pcbEnReady);
		sem_wait(&cpuDisponible);

		log_info(logger, "[readyAExe]: .........se despierta readyAExe ");
		log_info(logger, "[readyAExe]: a partir de ahora, CPU NO DISPONIBLE! ");
		t_pcb * procesoAEjecutar = malloc(sizeof(t_pcb));
		pthread_mutex_lock(&obtenerProceso);
		procesoAEjecutar = obtenerSiguienteDeReady();
		pthread_mutex_unlock(&obtenerProceso);

		log_info(loggerAux,
				"[readyAExe]: Desde readyAExe mandamos al proceso de pid %d a ejecutar  ",
				procesoAEjecutar->idProceso);

		procesoAEjecutar->estado = EXEC;

		log_info(logger,"PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXE>", procesoAEjecutar->idProceso);
		enviar_pcb(procesoAEjecutar,KERNEL_PCB_A_CPU,conexionCpuDispatch);
		//free(procesoAEjecutar);
		log_info(logger, "[readyAExe]: enviamos el proceso a cpu");

		if(!strcmp(algoritmoPlanificacion,"RR") || (!strcmp(algoritmoPlanificacion,"Feedback") && procesoAEjecutar->algoritmoActual == RR)){
			pthread_create(&hiloQuantumRR, NULL,(void*) controlar_quantum,NULL);
			pthread_detach(hiloQuantumRR);
		}

		log_info(logger,"[readyAExe]: finaliza");
	}
}


t_pcb* obtenerSiguienteDeReady() {

t_pcb * procesoPlanificado;  // = malloc(sizeof(t_pcb));
	log_info(loggerAux,"Algoritmo de plani: %s antes de entrar al if",algoritmoPlanificacion);

	if (!strcmp(algoritmoPlanificacion,"FIFO")) {
		log_info(loggerAux,"Entra al if por fifo");
		procesoPlanificado = obtenerSiguienteFIFO(); //REVISAR TEMA DE MALLOCS QUE EN SIGUIENTE RR ESTA COMENTADO
	} else {
		if (!strcmp(algoritmoPlanificacion, "RR")) {
			log_info(loggerAux,"Entra al if por rr");
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

				log_info(loggerAux, "[obtenerSiguienteDeReady]: algoritmo invalido");
			}
		}
	}
	return procesoPlanificado;
}


t_pcb* obtenerSiguienteFIFO() {

	t_pcb* procesoPlanificado = malloc(sizeof(t_pcb));
	procesoPlanificado = queue_pop(colaReadyFIFO);

	log_info(loggerAux, "[obtenerSiguienteFIFO]: PROCESOS EN READY FIFO: %d \n", queue_size(colaReadyFIFO));

	return procesoPlanificado;
}

t_pcb* obtenerSiguienteRR() {



	t_pcb* procesoPlanificado;   //= malloc(sizeof(t_pcb));
	procesoPlanificado = queue_pop(colaReadyRR);

	log_info(loggerAux, "[obtenerSiguienteFIFO]: PROCESOS EN READY RR: %d \n", queue_size(colaReadyRR));

	return procesoPlanificado;
}


void atender_interrupcion_de_ejecucion() {
	log_info(logger,"se desperto atender_interrupcion_de_ejecucion");
while (1) {

	int cod_op = recibir_operacion(conexionCpuDispatch); //conexionCpu es el socket interrupt

	switch (cod_op) {

	case CPU_PCB_A_KERNEL_POR_IO:
		t_pcb * proceso = recibir_pcb(conexionCpuDispatch);

		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion, "Feedback") && proceso->algoritmoActual == RR)){
			pthread_cancel(hiloQuantumRR);
			}

		char* dispositivo;

		int operacionDePcb = recibir_operacion(conexionCpuDispatch);
		if(operacionDePcb == CPU_DISPOSITIVO_A_KERNEL){
			dispositivo = recibir_mensaje(conexionCpuDispatch);
		}

		bool comparar_nombre_dispositivo( t_elem_disp * elemento){
			return !strcmp(elemento->dispositivo,dispositivo);
		}

		t_elem_disp * elementoLista = list_find(listaDeColasDispositivos, comparar_nombre_dispositivo);

		queue_push(elementoLista->cola_procesos,proceso);


		cod_op = recibir_operacion(conexionCpuDispatch);
		int unidadesTrabajo = 0;

		if(cod_op == CPU_A_KERNEL_UNIDADES_DE_TRABAJO_IO){
			unidadesTrabajo = recibir_entero(conexionCpuDispatch);
		}


		queue_push(elementoLista->cola_UTs,&unidadesTrabajo);

		log_info(logger, "PID: <%d> - Bloqueado por: <%s>" , proceso->idProceso, elementoLista->dispositivo);

		log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <BLOCKED>", proceso->idProceso);

		sem_post(&elementoLista->semaforo);


		break;
	case CPU_PCB_A_KERNEL_POR_IO_TECLADO:


		t_info_teclado * aMandarTeclado = malloc(sizeof(t_info_teclado));
		aMandarTeclado->pcb = recibir_pcb(conexionCpuDispatch);


		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion, "Feedback") && aMandarTeclado->pcb->algoritmoActual == RR)){

			pthread_cancel(hiloQuantumRR);
		}
		cod_op = recibir_operacion(conexionCpuDispatch);


		if(cod_op == CPU_A_KERNEL_INGRESAR_VALOR_POR_TECLADO){
			aMandarTeclado->registro =recibir_entero(conexionCpuDispatch);
		}

		

		log_info(logger, "PID: <%d> - Bloqueado por: <TECLADO>", proceso->idProceso);

		log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <BLOCKED>", proceso->idProceso);

		pthread_t hiloAtenderTeclado;
		pthread_create(&hiloAtenderTeclado, NULL,(void*) atender_IO_teclado,aMandarTeclado);
		pthread_detach(hiloAtenderTeclado);

		//free(aMandar.pcb);
		//free(aMandar.registro);
		//free(aMandar);

		break;

	case CPU_PCB_A_KERNEL_POR_IO_PANTALLA:

		t_info_pantalla * aMandarPantalla = malloc(sizeof(t_info_pantalla));

	//	pthread_mutex_lock(&mutexPantalla);
		aMandarPantalla->pcb = recibir_pcb(conexionCpuDispatch);

		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion,"Feedback") && aMandarPantalla->pcb->algoritmoActual == RR)){
						pthread_cancel(hiloQuantumRR);
					}


		cod_op = recibir_operacion(conexionCpuDispatch);

		if(cod_op == CPU_A_KERNEL_MOSTRAR_REGISTRO_POR_PANTALLA){
			aMandarPantalla->registro = recibir_entero(conexionCpuDispatch);
		}


	//	pthread_mutex_unlock(&mutexPantalla);

		log_info(logger, "PID: <%d> - Bloqueado por: <PANTALLA>", proceso->idProceso);

		log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <BLOCKED>", proceso->idProceso);

		pthread_t hiloPantalla;
		pthread_create(&hiloPantalla, NULL, &atender_IO_pantalla, aMandarPantalla);
		pthread_detach(hiloPantalla);

		break;


	case CPU_A_KERNEL_PCB_POR_DESALOJO:
		t_pcb* pcb = recibir_pcb(conexionCpuDispatch);


		if (!strcmp(algoritmoPlanificacion, "RR")) {
			queue_push(colaReadyRR,pcb);
		} else {
			if (!strcmp(algoritmoPlanificacion, "Feedback")) {
				queue_push(colaReadyFIFO,pcb);
			} else {
					log_info(loggerAux, "Algoritmo invalido");
			}
		}

		log_info(logger,"PID: <%d> - Desalojado por fin de Quantum", pcb->idProceso);

		log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <READY>", pcb->idProceso);

		sem_post(&pcbEnReady);

		break;

	case CPU_A_KERNEL_PCB_PAGE_FAULT:

		t_pcb* pcbPF = recibir_pcb(conexionCpuDispatch);

		int operacion = recibir_operacion(conexionCpuDispatch); //ver si es dispatch o interrupt
		if (operacion != CPU_A_KERNEL_PAGINA_PF){
			log_info(loggerAux, "codigo de operacion incorrecto");
		}

		t_list * listaTPyNroPag = list_create();
		listaTPyNroPag = recibir_lista_enteros(conexionCpuDispatch); //Nro de la pagina y nro de segmento que esta mal

		log_info(logger,"Numero de tabla de paginas: %d",list_get(listaTPyNroPag,0));
		log_info(logger,"Numero de paginas: %d",list_get(listaTPyNroPag,1));

		t_info_pf * aMandarPF = malloc(sizeof(t_info_pf));

		aMandarPF->pcb = pcbPF;
		aMandarPF->listaTpYNroPAgina = listaTPyNroPag;

		log_info(logger, "Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>",
				pcbPF->idProceso, list_get(listaTPyNroPag,1), list_get(listaTPyNroPag,0));

		log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <BLOCKED>", pcbPF->idProceso);

		pthread_t hiloPF;
		pthread_create(&hiloPF, NULL, &atender_page_fault, aMandarPF);
		pthread_detach(hiloPF);

		//free(aMandarPF);  DUDOSO
		break;

	case CPU_PCB_A_KERNEL_PCB_POR_FINALIZACION:
		log_info(loggerAux,"[atender_interrupcion_de_ejecucion]: recibimos operacion EXIT");

		t_pcb* pcbATerminar = recibir_pcb(conexionCpuDispatch);

		if(!strcmp(algoritmoPlanificacion, "RR") || (!strcmp(algoritmoPlanificacion, "Feedback") && pcbATerminar->algoritmoActual == RR)){
			pthread_cancel(hiloQuantumRR);
		}

		terminarEjecucion(pcbATerminar);
		break;
	default:
		log_info(loggerAux, "operacion invalida");
		sem_t spreenMiCasita;
		sem_init(&spreenMiCasita,0,0);
		sem_wait(&spreenMiCasita);
		break;
	}

	//ejecucionActiva = false;
	sem_post(&cpuDisponible);

	}
}

void terminarEjecucion(t_pcb * procesoAFinalizar) {
	procesoAFinalizar->estado = TERMINATED; // es local no hace falta mutex
	enviar_mensaje("Liberar estructuras",socketMemoria,KERNEL_A_MEMORIA_MENSAJE_LIBERAR_POR_TERMINADO);//Enviamos mensaje a memoria para que libere estructuras

	enviar_entero(procesoAFinalizar->idProceso, socketMemoria, KERNEL_A_MEMORIA_PID_PARA_FINALIZAR);

	log_info(loggerAux, "[terminarEjecucion]: Se envia un mensaje a memoria para que libere estructuras");

	enviar_mensaje("Finalizar consola", procesoAFinalizar->socket,KERNEL_MENSAJE_FINALIZAR_CONSOLA);

	liberar_conexion(procesoAFinalizar->socket);

	log_info(loggerAux, "[terminarEjecucion]: Aumenta grado de multiprogramacion actual y termina terminarEjecucion!!");

	log_info(logger,"PID: <%d> - Estado Anterior: <EXE> - Estado Actual: <TERMINATED>", procesoAFinalizar->idProceso);

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
					log_info(loggerAux,"codigo de operacion incorrecto");
				}
		recibir_mensaje(unPcb->socket);

		codigo = recibir_operacion(unPcb->socket);

		if(codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO){
						log_info(loggerAux,"codigo de operacion incorrecto");
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
					log_info(loggerAux, "Algoritmo invalido");
				}
			}
		}

		log_info(logger,"PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", unPcb->idProceso);

		sem_post(&pcbEnReady);
}

void atender_IO_pantalla(t_info_pantalla * info) {
	t_pcb * unPcb = info->pcb;

	int valor_registro = buscar_valor_registro(unPcb ,info->registro);

	enviar_entero(valor_registro, unPcb->socket, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

	recibir_operacion(unPcb->socket);//recibir con cod_op

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
				log_info(loggerAux, "Algoritmo invalido");
			}
		}
	}

	log_info(logger,"PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", unPcb->idProceso);

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

	enviar_mensaje("Cpu desalojá tu proceso por fin de quantum", conexionCpuInterrupt ,DESALOJAR_PROCESO_POR_FIN_DE_QUANTUM); // ES EL SOCKET

}

void atender_IO_generico(t_elem_disp* elemento){
	log_info(logger,"se desperto atender_IO_generico del dispositivo <%s> por primera vez ", elemento->dispositivo);

	while(1) {
		sem_wait(&elemento->semaforo);
		log_info(logger," [atender_IO_generico del dispositivo <%s>] : se desperto ", elemento->dispositivo);

		int * unidades_trabajo = queue_pop(elemento->cola_UTs);
		int tiempo_dispositivo = elemento->tiempo;
		log_info(logger," [atender_IO_generico del dispositivo <%s>] : el tiempo de dispositivo es %d y las unidades de trabajo obtenidas de la cola son %d ", elemento->dispositivo, tiempo_dispositivo,*unidades_trabajo);
		log_info(logger," [atender_IO_generico del dispositivo <%s>] : simulando tiempo de bloqueo  ", elemento->dispositivo);

		usleep(tiempo_dispositivo * 1000 * (*unidades_trabajo));



		t_pcb * proceso = queue_pop(elemento->cola_procesos);

		log_info(logger," [atender_IO_generico del dispositivo <%s>] : obtuvimos el siguiente PCB  ", elemento->dispositivo);

		controlar_pcb(proceso);

		if (!strcmp(algoritmoPlanificacion, "FIFO")) {
			queue_push(colaReadyFIFO,proceso);
		} else {
			if (!strcmp(algoritmoPlanificacion, "RR")) {
				queue_push(colaReadyRR,proceso);
			} else {
				if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
					queue_push(colaReadyRR,proceso);
				}else{
					log_info(loggerAux, "Algoritmo invalido");
				}
			}
		}

		log_info(logger,"PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", proceso->idProceso);

		sem_post(&pcbEnReady);
	}


}



void atender_page_fault(t_info_pf* infoPF){

	t_pcb * pcb = infoPF->pcb;
	t_list * listaTPyNroPag = infoPF->listaTpYNroPAgina;

	pcb->estado = BLOCKED; // hace falta un nuevo estado BLOCKED_PF?
	log_info(loggerAux,"Antes mensaje");
	enviar_mensaje("kernel solicita que se cargue en memoria la pagina correspondiente",socketMemoria, KERNEL_MENSAJE_SOLICITUD_CARGAR_PAGINA);
	log_info(loggerAux,"Antes mensaje");
	t_paquete * paquete = crear_paquete(KERNEL_A_MEMORIA_PAGE_FAULT);

	int numeroDelSegmento = list_get(listaTPyNroPag,1);
	int numeroTablaPagina;
	entradaTablaSegmento* unaEntradaSegmento = malloc(sizeof(entradaTablaSegmento));

	for(int i=0;i<list_size(pcb->tablaSegmentos);i++){
		unaEntradaSegmento = list_get(pcb->tablaSegmentos,i);
		if(numeroDelSegmento == unaEntradaSegmento->numeroSegmento){
			numeroTablaPagina = unaEntradaSegmento->numeroTablaPaginas;
		}
	}

	int numeroDePagina = list_get(listaTPyNroPag,0);
			agregar_a_paquete_unInt(paquete, &numeroTablaPagina, sizeof(int));
			agregar_a_paquete_unInt(paquete, &numeroDePagina, sizeof(int));
			enviar_paquete(paquete,socketMemoria);
			eliminar_paquete(paquete);


	int codigo = recibir_operacion(socketMemoria);

	if(codigo != KERNEL_MENSAJE_CONFIRMACION_PF){
		log_info(loggerAux,"codigo de operacion incorrecto");
	}

	recibir_mensaje(socketMemoria);

	if (!strcmp(algoritmoPlanificacion, "FIFO")) {
		queue_push(colaReadyFIFO,pcb);
	} else {
		if (!strcmp(algoritmoPlanificacion, "RR")) {
			queue_push(colaReadyRR,pcb);
		} else {
			if(!strcmp(algoritmoPlanificacion, "Feedback")) {// ver si esta asi en los config
				queue_push(colaReadyRR,pcb);
			}else{
				log_info(loggerAux, "Algoritmo invalido");
			}
		}
	}

	log_info(logger,"PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", pcb->idProceso);

	sem_post(&pcbEnReady);

}


t_pcb* recibir_pcb(int socket_cliente)//ponerla en shared
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = list_create();
	pcb->tablaSegmentos = list_create();
	int contadorInstrucciones = 0;
	int contadorSegmentos = 0;
	int i = 0;
	memcpy(&pcb->idProceso, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	log_info(logger,"----------ME LLEGO EL SIGUIENTE PCB-------------");
	log_info(loggerAux,"PID: %d",pcb->idProceso);
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
		//log_info(loggerAux,"Instruccion: %s",unaInstruccion->identificador);
		//log_info(loggerAux,"Primer parametro: %d",unaInstruccion->parametros[0]);
		i++;
	}
	memcpy(&pcb->programCounter, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	log_info(loggerAux,"Con program counter: %d",pcb->programCounter);
	memcpy(&pcb->registros, buffer + desplazamiento, sizeof(t_registros));
	desplazamiento+=sizeof(t_registros);
	//log_info(loggerAux,"Registro AX: %d y DX: %d",pcb->registros.AX,pcb->registros.DX);
	memcpy(&contadorSegmentos, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	i=0;
	//log_info(loggerAux,"cont seg: %d",contadorSegmentos);
	while(i < contadorSegmentos){
		entradaTablaSegmento* unSegmento = malloc(sizeof(entradaTablaSegmento));
		memcpy(&(unSegmento->numeroSegmento),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&(unSegmento->numeroTablaPaginas),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&(unSegmento->tamanioSegmento),buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		//log_info(loggerAux,"Nro: %d, numero tabla paginas %d, tamanio segmento %d",unSegmento->numeroSegmento,unSegmento->numeroTablaPaginas,unSegmento->tamanioSegmento);
		list_add(pcb->tablaSegmentos,unSegmento);
		i++;
	}
	memcpy(&pcb->socket, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(&(pcb->estado), buffer + desplazamiento, sizeof(t_estado));
	desplazamiento+=sizeof(t_estado);
	memcpy(&pcb->algoritmoActual, buffer + desplazamiento, sizeof(t_algoritmo_pcb));
	desplazamiento+=sizeof(int);
	//log_info(loggerAux,"Estado: %d",pcb->estado);
	free(buffer);
	return pcb;
}




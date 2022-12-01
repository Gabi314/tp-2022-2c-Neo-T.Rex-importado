#include "funcionesCpu.h"


//--------------DECLARO VARIABLES
int clienteKernel;
int clienteKernelInterrupt;
int socket_memoria;
char* ipMemoria;
char* puertoMemoria;
char* puertoDeEscuchaDispatch;
char* puertoDeEscuchaInterrupt;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int contadorSegmentos;
int desplazamiento;
t_paquete* paquete;

//-----------------------FUNCIONES
int conexionConKernelDispatch(void){//un hilo
	static pthread_mutex_t mutexMensajes;// usar

	int server_dispatch = iniciar_servidor(IP_CPU,puertoDeEscuchaDispatch,"Kernel");
	log_info(logger,"Cpu lista para recibir a kernel");
	clienteKernel = esperar_cliente(server_dispatch,"Kernel");

	int server_interrupt = iniciar_servidor(IP_CPU, puertoDeEscuchaInterrupt,"Kernel");
	clienteKernelInterrupt = esperar_cliente(server_interrupt, "Kernel por interrupt");

	//int salirDelWhile = 0;
	while (1) {
		int cod_op = recibir_operacion(clienteKernel);

		if(cod_op == KERNEL_PCB_A_CPU){

			unPcb = recibir_pcb(clienteKernel);

			return EXIT_SUCCESS;
		}else if(cod_op == -1){
			log_info(logger, "Se desconecto el kernel. Terminando conexion");
			return EXIT_SUCCESS;
		}
	}
	return EXIT_SUCCESS;
}

int conexionConMemoria(void){

	// Creamos una conexión hacia el servidor
    socket_memoria = crear_conexion(ipMemoria, puertoMemoria);
	log_info(logger,"Hola memoria, soy cpu");

	enviar_mensaje("Dame el tamanio de pag y entradas por tabla",socket_memoria,MENSAJE_CPU_MEMORIA);//MENSAJE_PEDIDO_CPU

	t_list* listaQueContieneTamanioDePagYEntradas = list_create();

	int cod_op = recibir_operacion(socket_memoria);

	if(cod_op == TAM_PAGINAS_Y_CANT_ENTRADAS){
		listaQueContieneTamanioDePagYEntradas = recibir_lista_enteros(socket_memoria);
	}

	leerTamanioDePaginaYCantidadDeEntradas(listaQueContieneTamanioDePagYEntradas);

	return EXIT_SUCCESS;
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
		log_info(logger,"Primer parametro: %d",unaInstruccion->parametros[0]);
		i++;
	}
	memcpy(&pcb->programCounter, buffer + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	log_info(logger,"program counter: %d",pcb->programCounter);
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
		list_add(pcb->tablaSegmentos,unSegmento);
		i++;
	}
//	memcpy(&pcb->socket_cliente, buffer + desplazamiento, sizeof(int)); al pedo mepa
	memcpy(&(pcb->estado), buffer + desplazamiento, sizeof(t_estado));
	desplazamiento+=sizeof(t_estado);
	log_info(logger,"Estado: %d",pcb->estado);
	free(buffer);
	return pcb;
}


//----------------------------- Para ser cliente de Memoria -------------------------------------------------

//ESTO ES PARA MANDAR UN PCB A KERNEL-------------------------------------------------------------------------------------
void enviar_pcb(t_pcb* pcb,int cod_op,int clienteKernel)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	contadorSegmentos = 0;
	desplazamiento = 0;
	t_paquete* paquete = crear_paquete(cod_op);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	list_iterate(pcb->tablaSegmentos, (void*) obtenerCantidadDeSegmentos);
	paquete->buffer->stream = realloc(paquete->buffer->stream,
			paquete->buffer->size + tamanioTotalIdentificadores
					+ contadorInstrucciones * sizeof(int[2])
					+ contadorInstrucciones * sizeof(int) + 3 * sizeof(int)
					+ sizeof(t_registros)
					+ contadorSegmentos * sizeof(entradaTablaSegmento)
					+ sizeof(int));
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

	enviar_paquete(paquete, clienteKernel);
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
	void* id = unaInstruccion->identificador;
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

void enviarNroTablaDePaginas(t_list* tablaDeSegmentos,int numeroDeSegmento, int socket_memoria, int numeroDePagina) {
	entradaTablaSegmento* unaEntradaTablaSegmento = malloc(sizeof(entradaTablaSegmento));
	unaEntradaTablaSegmento = list_get(tablaDeSegmentos,numeroDeSegmento);

	log_info(logger,"Le envio a memoria nro tabla de paginas %d y la pagina buscada %d",
			unaEntradaTablaSegmento->numeroTablaPaginas, numeroDePagina);

	t_paquete* paquete = crear_paquete(PRIMER_ACCESO);

	agregar_a_paquete_unInt(paquete, &unaEntradaTablaSegmento->numeroTablaPaginas, sizeof(int));
	agregar_a_paquete_unInt(paquete, &numeroDePagina, sizeof(int));

	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

void enviarDireccionFisica(int marco, int desplazamientoPagina,int leer,int valorDelRegistro){
	t_paquete* paquete;
	if(leer){
		paquete = crear_paquete(CPU_A_MEMORIA_LEER);
		agregar_a_paquete_unInt(paquete,&marco,sizeof(marco));
		agregar_a_paquete_unInt(paquete,&desplazamientoPagina,sizeof(desplazamientoPagina));
	}else{
		paquete = crear_paquete(CPU_A_MEMORIA_VALOR_A_ESCRIBIR);
		agregar_a_paquete_unInt(paquete,&marco,sizeof(marco));
		agregar_a_paquete_unInt(paquete,&desplazamientoPagina,sizeof(desplazamientoPagina));
		agregar_a_paquete_unInt(paquete,&valorDelRegistro,sizeof(valorDelRegistro));
		log_info(logger,"Le envio a memoria el valor del registro: %d a escribir",valorDelRegistro);
	}

	log_info(logger,"Le envio a memoria direccion fisica: Marco:%d y Offset: %d",marco,desplazamiento);
	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);
}

void enviarValorAEscribir(uint32_t valorAEscribir){
	log_info(logger,"Le envio a memoria el valor a escribir %u",valorAEscribir);
	enviar_entero(valorAEscribir, socket_memoria, CPU_A_MEMORIA_VALOR_A_ESCRIBIR);
}


void iterator(instruccion* unaInstruccion) {
	log_info(logger,"%s", unaInstruccion->identificador);
}

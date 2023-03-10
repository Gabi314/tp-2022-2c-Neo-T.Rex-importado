#include "funcionesCpu.h"

bool ejecutando;
pthread_t hilo_dispatch, hilo_interrupt;
pthread_mutex_t mutexEjecutar;
int server_dispatch;
int server_interrupt;

int main(int argc, char *argv[]) {

	logger = log_create("cpu-auxiliar.log", "CPU-AUX", 1, LOG_LEVEL_INFO);
	loggerObligatorio = log_create("cpu.log","CPU",1,LOG_LEVEL_INFO);

	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	inicializarConfiguraciones(argv[1]);

	inicializarTLB();

	server_dispatch = iniciar_servidor(IP_CPU, puertoDeEscuchaDispatch, "Kernel");
	log_info(logger,"Servidor CPU-DISPATCH:[%d] iniciado", server_dispatch);

	server_interrupt = iniciar_servidor(IP_CPU, puertoDeEscuchaInterrupt,"Kernel");
	log_info(logger,"Servidor CPU-INTERRUPT:[%d] iniciado", server_interrupt);

	/*
	pthread_t hiloEjecutar;
	//ejecutando = true;// ver donde poner mejor esto
	pthread_create(&hiloEjecutar, NULL, (void*) ejecucion, NULL);
	pthread_detach(hiloEjecutar);
	*/

	crear_hilos_servidor_cpu();

	handshakeMemoria();

	sem_t cpuSinFinalizar;
	sem_init(&cpuSinFinalizar,0,0);
	sem_wait(&cpuSinFinalizar);

	//free(unPcb);
}

void ejecucion(void* aa){

	instruccion* unaInstruccion = malloc(sizeof(instruccion));

	while (1) {

		//pthread_mutex_lock(&mutexEjecutar);
		sem_wait(&pcbRecibido);
		//if(ejecutando){
			//pthread_mutex_unlock(&mutexEjecutar);
			unaInstruccion = buscarInstruccionAEjecutar(pcb);
			log_info(logger,"Instruccion a ejecutar %s",unaInstruccion->identificador);
			ejecutar(unaInstruccion, pcb);
			if(ejecutando){
				sem_post(&pcbRecibido);
				//unaInstruccion = buscarInstruccionAEjecutar(unPcb);
			}
		//}else {
			//pthread_mutex_unlock(&mutexEjecutar);
		//}

	}
}

void crear_hilos_servidor_cpu() {
	pthread_create(&hilo_dispatch, NULL, (void*) conexion_kernel_dispatch, NULL);
	pthread_detach(hilo_dispatch);

	pthread_create(&hilo_interrupt, NULL, (void*) conexion_kernel_interrupt, NULL);
	pthread_detach(hilo_interrupt);
}

//	unPcb = malloc(sizeof(t_pcb));
//
//	unPcb->idProceso = 0; //PARA HACER PRUEBAS SIN KERNEL
//
//	instruccion* instruccion1 = malloc(sizeof(instruccion));
//	instruccion1->identificador = "SET";
//	instruccion1->parametros[0]=AX;
//	instruccion1->parametros[1]=10;
//
//	instruccion* instruccion2 = malloc(sizeof(instruccion));
//	instruccion2->identificador = "SET";
//	instruccion2->parametros[0]=BX;
//	instruccion2->parametros[1]=12;
//
//	instruccion* instruccion3 = malloc(sizeof(instruccion));
//	instruccion3->identificador = "ADD";
//	instruccion3->parametros[0]=AX;
//	instruccion3->parametros[1]=BX;
//
//	instruccion* instruccion4 = malloc(sizeof(instruccion));
//	instruccion4->identificador = "MOV_OUT";
//	instruccion4->parametros[0] = 0;
//	instruccion4->parametros[1]= AX;
//
//	instruccion* instruccion5 = malloc(sizeof(instruccion));
//	instruccion5->identificador = "MOV_IN";
//	instruccion5->parametros[0] = CX;
//	instruccion5->parametros[1]= 0;
//
//	instruccion* instruccion6 = malloc(sizeof(instruccion));
//	instruccion6->identificador = "EXIT";
//
//	unPcb->instrucciones = list_create();
//
//	list_add(unPcb->instrucciones,instruccion1);
//	list_add(unPcb->instrucciones,instruccion2);
//	list_add(unPcb->instrucciones,instruccion3);
//	list_add(unPcb->instrucciones,instruccion4);
//	list_add(unPcb->instrucciones,instruccion5);
//	list_add(unPcb->instrucciones,instruccion6);
//
//	unPcb->programCounter = 0;
//	unPcb->registros.AX = 0;
//	unPcb->registros.BX = 0;
//	unPcb->registros.CX = 0;
//	unPcb->registros.DX = 0;
//
//	entradaTablaSegmento* unaEntradaTS = malloc(sizeof(entradaTablaSegmento));
//
//	unaEntradaTS->numeroSegmento = 0;
//	unaEntradaTS->numeroTablaPaginas = 0;
//	unaEntradaTS->tamanioSegmento = 64;
//
//	unPcb->tablaSegmentos = list_create();
//
//	list_add(unPcb->tablaSegmentos,unaEntradaTS);


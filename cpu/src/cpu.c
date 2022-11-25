#include "funcionesCpu.h"

bool ejecutando;
char** listaDispositivos;

int main(int argc, char *argv[]) {

	logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_INFO);

	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	inicializarConfiguraciones(argv[1]);

	tlb = inicializarTLB();

	unPcb = malloc(sizeof(t_pcb));
	unPcb->idProceso = 0;

	instruccion* instruccion1 = malloc(sizeof(instruccion));
	instruccion1->identificador = "SET";
	instruccion1->parametros[0]=AX;
	instruccion1->parametros[1]=10;

	instruccion* instruccion2 = malloc(sizeof(instruccion));
	instruccion2->identificador = "SET";
	instruccion2->parametros[0]=BX;
	instruccion2->parametros[1]=12;

	instruccion* instruccion3 = malloc(sizeof(instruccion));
	instruccion3->identificador = "ADD";
	instruccion3->parametros[0]=AX;
	instruccion3->parametros[1]=BX;

	instruccion* instruccion4 = malloc(sizeof(instruccion));
	instruccion4->identificador = "MOV_OUT";
	instruccion4->parametros[0] = 0;
	instruccion4->parametros[1]= AX;

	instruccion* instruccion5 = malloc(sizeof(instruccion));
	instruccion5->identificador = "MOV_IN";
	instruccion5->parametros[0] = CX;
	instruccion5->parametros[1]= 0;

	instruccion* instruccion6 = malloc(sizeof(instruccion));
	instruccion6->identificador = "EXIT";

	unPcb->instrucciones = list_create();

	list_add(unPcb->instrucciones,instruccion1);
	list_add(unPcb->instrucciones,instruccion2);
	list_add(unPcb->instrucciones,instruccion3);
	list_add(unPcb->instrucciones,instruccion4);
	list_add(unPcb->instrucciones,instruccion5);
	list_add(unPcb->instrucciones,instruccion6);

	unPcb->programCounter = 0;
	unPcb->registros.AX = 0;
	unPcb->registros.BX = 0;
	unPcb->registros.CX = 0;
	unPcb->registros.DX = 0;

	entradaTablaSegmento* unaEntradaTS = malloc(sizeof(entradaTablaSegmento));

	unaEntradaTS->numeroSegmento = 0;
	unaEntradaTS->numeroTablaPaginas = 0;
	unaEntradaTS->tamanioSegmento = 64;

	unPcb->tablaSegmentos = list_create();

	list_add(unPcb->tablaSegmentos,unaEntradaTS);

	//conexionConKernelDispatch(); //Recibo pcb

	//listaDispositivos = recibirListaDispositivos(clienteKernel);
	char** listaDispositivos = {"DISCO","IMPRESORA"};

//	strcpy(palabra[0],"DISCO");
//	strcpy(palabra[1],"IMPRESORA");



	conexionConMemoria();



	checkInterrupt();

	pthread_t hiloEjecutar;
	ejecutando = true;// ver donde poner mejor esto
	pthread_create(&hiloEjecutar, NULL, (void*) ejecucion,
				NULL);
	//pthread_detach(hiloEjecutar);


	log_info(logger,"boca");
	//pthread_join(hiloInterrupciones,NULL);
	pthread_join(hiloEjecutar,NULL);

	free(unPcb);

	return EXIT_SUCCESS;

}

void ejecucion(){
	instruccion *unaInstruccion = malloc(sizeof(instruccion));
	unaInstruccion = buscarInstruccionAEjecutar(unPcb);

	while (ejecutando) {
		ejecutar(unaInstruccion, unPcb);
		unaInstruccion = buscarInstruccionAEjecutar(unPcb);

	}

	free(unaInstruccion);
}

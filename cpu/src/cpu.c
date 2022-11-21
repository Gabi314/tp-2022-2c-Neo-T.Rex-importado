#include "funcionesCpu.h"

bool ejecutando;
char** listaDispositivos;

int main(int argc, char *argv[]) {

	logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_INFO);

	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	inicializarConfiguraciones(argv[1]);

	unPcb = malloc(sizeof(t_pcb));
	conexionConKernelDispatch(); //Recibo pcb

	listaDispositivos = recibirListaDispositivos(clienteKernel);

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

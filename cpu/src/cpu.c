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

	instruccion *unaInstruccion = malloc(sizeof(instruccion));
	unaInstruccion = buscarInstruccionAEjecutar(unPcb);

	ejecutando = true;
	while (ejecutando) {
		ejecutar(unaInstruccion, unPcb);
		unaInstruccion = buscarInstruccionAEjecutar(unPcb);
	}
	free(unPcb);
	free(unaInstruccion);
	return EXIT_SUCCESS;

}

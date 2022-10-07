#include "funcionesCpu.h"

int main(int argc, char *argv[]){

	logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_INFO);

	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	inicializarConfiguraciones(argv[1]);

	unPcb = malloc(sizeof(t_pcb));

	conexionConKernel();//Recibo pcb


}

#include "funcionesMemoria.h"

int memoria_fd;
t_list* listaDeMarcos;
t_list* listaDeEntradasEnMemoria;
t_list* listaTablaDePaginas;


int main(int argc, char *argv[]) {

	//OBLIGATORIO
	funcionMain(argc, argv);
	//OBLIGATORIO

	memoria_fd = iniciar_servidor(IP_MEMORIA, puertoMemoria, "MEMORIA");
	while(1) {
		// Escucho a cliente CPU
		server_escuchar(loggerAux, "MEMORIA", "CPU", memoria_fd);
		// Escucho a cliente Kernel
		server_escuchar(loggerAux, "MEMORIA", "KERNEL", memoria_fd);
	}

//PROBAR ENTRADAS CON BIT DE MODIFICADO VARIADO

	//conexionConCpu();

	log_info(loggerAux,"Fin de memoria");
	log_info(loggerAux,"Boca");
}

void funcionMain(int argc, char *argv[]) {
	// Chequeo de parametros
	chequeoCantidadArchivos(argc);

	//Logs obligatorios
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	// Logs auxiliares
	loggerAux = log_create("memoriaAux.log", "MEMORIA", 1, LOG_LEVEL_INFO);

	crearConfiguraciones(argv[1]);

	listaDeMarcos = list_create();
	listaDeEntradasEnMemoria = list_create();
	listaTablaDePaginas = list_create();

	inicializarMemoria();
	inicializarMarcos();

	crearSwap();

	//listaDePaginasEnMemoria = list_create();

	log_info(loggerAux, "Fin de main obligatorio");
}





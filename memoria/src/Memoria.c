#include "funcionesMemoria.h"

int memoria_fd;

int main(int argc, char *argv[]) {

	//OBLIGATORIO
	funcionMain(argc, argv);
	//OBLIGATORIO

	memoria_fd = iniciar_servidor(IP_MEMORIA, puertoMemoria, "MEMORIA");
	while(1) {
		// Escucho a cliente Kernel
		server_escuchar(loggerAux, "MEMORIA", "KERNEL", memoria_fd);

		// Escucho a cliente CPU
		server_escuchar(loggerAux, "MEMORIA", "CPU", memoria_fd);
	}

	// Organizar

	cantidadDeSegmentos = 4;
	inicializarEstructuras(0);

	log_info(loggerAux, "Tamaño de la lista tabla de paginas: %d", list_size(listaTablaDePaginas));

	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	unaTablaDePaginas = list_get(listaTablaDePaginas,0);

	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	unaEntrada = list_get(unaTablaDePaginas->entradas,0);
	entradaTablaPaginas* unaEntrada1 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada1 = list_get(unaTablaDePaginas->entradas,1);
	entradaTablaPaginas* unaEntrada2 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada2 = list_get(unaTablaDePaginas->entradas,2);
	entradaTablaPaginas* unaEntrada3 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada3 = list_get(unaTablaDePaginas->entradas,3);

	tablaDePaginas* unaTablaDePaginas1 = malloc(sizeof(tablaDePaginas));
	unaTablaDePaginas1 = list_get(listaTablaDePaginas,1);
	entradaTablaPaginas* unaEntrada4 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada4 = list_get(unaTablaDePaginas1->entradas,0);
	entradaTablaPaginas* unaEntrada5 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada5 = list_get(unaTablaDePaginas1->entradas,1);
	entradaTablaPaginas* unaEntrada6 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada6 = list_get(unaTablaDePaginas1->entradas,2);
	entradaTablaPaginas* unaEntrada7 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada7 = list_get(unaTablaDePaginas1->entradas,3);

	tablaDePaginas* unaTablaDePaginas2 = malloc(sizeof(tablaDePaginas));
	unaTablaDePaginas2 = list_get(listaTablaDePaginas,2);
	entradaTablaPaginas* unaEntrada8 = malloc(sizeof(entradaTablaPaginas));
	unaEntrada8 = list_get(unaTablaDePaginas2->entradas,0);

	cargarPagina(unaEntrada);
	cargarPagina(unaEntrada1);
	cargarPagina(unaEntrada2);
	cargarPagina(unaEntrada3);

	cargarPagina(unaEntrada4);
	cargarPagina(unaEntrada5);
	cargarPagina(unaEntrada6);
	cargarPagina(unaEntrada7);

	cargarPagina(unaEntrada8);



//PROBAR ENTRADAS CON BIT DE MODIFICADO VARIADO

	//conexionConCpu();

	log_info(loggerAux,"Fin de memoria");
	log_info(loggerAux,"Boca");
}

void funcionMain(int argc, char *argv[]) {
	//Obligatorios
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO); //OBLIGATORIOS
	loggerAux = log_create("memoriaAux.log", "MEMORIA", 1, LOG_LEVEL_INFO); //AUX
	listaDeMarcos = list_create();
	listaDeEntradasEnMemoria = list_create();
	listaTablaDePaginas = list_create();
	//listaDePaginasEnMemoria = list_create();
	chequeoCantidadArchivos(argc);
	crearConfiguraciones(argv[1]);
	crearSwap();
	inicializarMemoria();
	inicializarMarcos();
	//conexionCpu();
	//Obligatorios
	log_info(loggerAux, "Fin de main obligatorio");
}





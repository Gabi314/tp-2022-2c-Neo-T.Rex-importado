#include "funcionesMemoria.h"

int memoria_fd_cpu;
int memoria_fd_kernel;
int memoria_fd;
t_list* listaDeMarcos;
t_list* listaTablaDePaginas;
t_list* lista_frames_procesos;

int main(int argc, char *argv[]) {

	//OBLIGATORIO
	funcionMain(argc, argv);
	//OBLIGATORIO

	memoria_fd = iniciar_servidor(IP_MEMORIA, puertoMemoria, "MEMORIA");

	while(1){
	// Escucho a cliente CPU
	server_escuchar(loggerAux, "MEMORIA", "CPU", memoria_fd);
	// Escucho a cliente Kernel
	server_escuchar(loggerAux, "MEMORIA", "KERNEL", memoria_fd);
	}

//	sem_t memoriaSinFinalizar;
//	sem_init(&memoriaSinFinalizar,0,0);
//	sem_wait(&memoriaSinFinalizar);
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
	listaTablaDePaginas = list_create();
	lista_frames_procesos = list_create();

	pthread_mutex_init(&conexionCpu,NULL);
	pthread_mutex_init(&conexionKernel,NULL);
	inicializarMemoria();
	inicializarMarcos();

	crearSwap();


	log_info(loggerAux, "Fin de main obligatorio");
}





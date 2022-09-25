#include <stdio.h>
#include <stdlib.h>
#include "funcionesKernel.h"


int main(int argc, char *argv[]) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}

	//sem_init(&kernelSinFinalizar,0,0);

	inicializarConfiguraciones(argv[1]);
	log_info(logger,"Iniciando conexion con consola");

	//socketServidor = iniciar_servidor();


	conexionConConsola();
/*
	inicializar_colas();
	inicializar_semaforos();


	pthread_t hilo0;
	pthread_t hiloAdmin[6];
	int hiloAdminCreado[6];

	ejecucionActiva = false;
	procesoDesalojado = NULL;
	//conexiones
	socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	socketCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	socketServidor = iniciar_servidor();

	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,socketServidor);
	pthread_detach(hiloCreado);

	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
	hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&atender_interrupcion_de_ejecucion,NULL); // problemas con esto
	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&atenderDesalojo,NULL);
	hiloAdminCreado[3] = pthread_create(&hiloAdmin[3],NULL,&readyAExe,NULL);
	hiloAdminCreado[4] = pthread_create(&hiloAdmin[4],NULL,&atenderIO,NULL);
	hiloAdminCreado[5] = pthread_create(&hiloAdmin[5],NULL,&desbloquear_suspendido,NULL);

	pthread_detach(hiloAdmin[0]);
	pthread_detach(hiloAdmin[1]);
	pthread_detach(hiloAdmin[2]);
	pthread_detach(hiloAdmin[3]);
	pthread_detach(hiloAdmin[4]);
	pthread_detach(hiloAdmin[5]);
*/

	log_destroy(logger);
	//sem_wait(&kernelSinFinalizar);

	//int nroTabla1erNivel = conexionConMemoria();
	//conexionConConsola();

	//cargar_pcb(nroTabla1erNivel);
	//conexionConCpu();

}



void inicializarConfiguraciones(char* unaConfig){
	t_config* config = config_create(unaConfig);
	if(config  == NULL){
		printf("Error leyendo archivo de configuración. \n");
	}

	ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");//son intss
	ipCpu = config_get_string_value(config, "IP_CPU");
	puertoCpuDispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");//son intss
	puertoCpuInterrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	puertoKernel = config_get_string_value(config, "PUERTO_ESCUCHA"); //no lo usamos
	algoritmoPlanificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	//gradoMultiprogramacionTotal = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	//dispositivos_io = config_get_array_value(config,"DISPOSITIVOS_IO")[1];
//	tiempos_io = config_get_array_value(config,"TIEMPOS_IO");
	//quantum_rr = config_get_int_value(config,"QUANTUM_RR");

}

int conexionConConsola(){

	int server_fd = iniciar_servidor();
	log_info(logger, "Kernel listo para recibir a consola");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* listaQueContieneTamSegmento;

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			/*case KERNEL_PAQUETE_INSTRUCCIONES:
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete(cliente_fd);
				log_info(logger,"me llegaron las instrucciones");
				//list_iterate(instrucciones, (void*) iterator);
			break;
			*/
				case KERNEL_PAQUETE_TAMANIOS_SEGMENTOS:
				listaQueContieneTamSegmento = list_create();
				listaQueContieneTamSegmento = recibir_lista_enteros(cliente_fd);//Hacer que reciba paquete vectorDeEnteros

				int tamanioDelSegmento = (int) list_get(listaQueContieneTamSegmento,3);
				log_info(logger,"El tamanio del primer segmento es: %d",tamanioDelSegmento);
				break;
			case -1:
				log_error(logger, "La consola se desconecto. Finalizando Kernel");
			return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
			}
	  return EXIT_SUCCESS;
}

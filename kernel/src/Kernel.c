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

	// Inicializaciones
	//inicializar_configuraciones(argv[1]);
	identificadores_pcb = 0;

	t_config* config = inicializar_configuracion(argv[1]);

	ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");//son intss
	ipCpu = config_get_string_value(config, "IP_CPU");
	puertoCpuDispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");//son intss
	puertoCpuInterrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	puertoKernel = config_get_string_value(config, "PUERTO_ESCUCHA"); //no lo usamos
	algoritmoPlanificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	gradoMultiprogramacionTotal = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	quantum_rr = config_get_int_value(config,"QUANTUM_RR");
	dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
	tiempos_io = config_get_array_value(config, "TIEMPOS_IO");


	inicializar_listas_y_colas();
	log_info(logger, "En la lista de dispositivos tenemos los siguientes:");
	for (int i = 0; i < list_size(listaDeColasDispositivos); i++) {
		t_elem_disp* aux = list_get(listaDeColasDispositivos,i);
		log_info(logger,"el dispositivo de posicion %d se llama %s", i , aux->dispositivo);
		log_info(logger,"el dispositivo de posicion %d tiene un tiempo de %d", i, aux->tiempo);
			}

	log_info(logger,"Iniciando conexion con consola");

	//socketServidor = iniciar_servidor();

	inicializar_semaforos();

	//t_pcb * PCB = conexionConConsola();
	//conexionConCpu(PCB);


/*
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


	//cargar_pcb(nroTabla1erNivel);
	//conexionConCpu();

}

void inicializar_configuraciones(char* unaConfig){
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
	gradoMultiprogramacionTotal = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	quantum_rr = config_get_int_value(config,"QUANTUM_RR");
	dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
	tiempos_io = config_get_array_value(config, "TIEMPOS_IO");
}

void inicializar_listas_y_colas() {
	inicializar_lista_dispositivos();

	colaNew = queue_create();
	colaReadyFIFO = queue_create();
	colaReadyRR = queue_create();
	listaDeColasDispositivos = list_create();

}

t_pcb * conexionConConsola(){

	int server_fd = iniciar_servidor();
	log_info(logger, "Kernel listo para recibir a consola");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* listaQueContieneTamSegmento;
	t_pcb * PCB = malloc(sizeof(t_pcb *));

	log_info(logger, "llego antes de recibir_operacion");

	int operacion = recibir_operacion(cliente_fd);

	log_info(logger,"se recibio la operacion %d", operacion);

	if (operacion == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS){
	//	listaQueContieneTamSegmento = list_create();
	//	listaQueContieneTamSegmento = recibir_lista_enteros(cliente_fd);
		PCB->tabla_segmentos = list_create();
		PCB->tabla_segmentos = inicializar_tabla_segmentos(cliente_fd); // aca usariamos el recibir_lista_enteros
	//	int tamanioDelSegmento = (int) list_get(listaQueContieneTamSegmento,3);
	//			log_info(logger,"El tamanio del primer segmento es: %d",tamanioDelSegmento);

		if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_INSTRUCCIONES){
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete_instrucciones(cliente_fd);
				log_info(logger,"me llegaron las instrucciones");
				list_iterate(listaInstrucciones, (void*) iterator);

				enviar_mensaje("segmentos e instrucciones recibidos pibe", cliente_fd,KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS);

				}
				else {
					log_info(logger,"codigo de operacion incorrecto");
				}
	}
	else {
		log_info(logger,"codigo de operacion incorrecto");
	}

	enviar_entero(10, cliente_fd, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

	int codigo = recibir_operacion(cliente_fd);
	if(codigo != KERNEL_MENSAJE_VALOR_IMPRESO){
			log_info(logger,"codigo de operacion incorrecto");
		}
	recibir_mensaje(cliente_fd);

	enviar_mensaje("kernel solicita que se ingrese un valor por teclado",cliente_fd,KERNEL_MENSAJE_SOLICITUD_VALOR_POR_TECLADO);

	codigo = recibir_operacion(cliente_fd);
		if(codigo != KERNEL_MENSAJE_DESBLOQUEO_TECLADO){
				log_info(logger,"codigo de operacion incorrecto");
			}
	recibir_mensaje(cliente_fd);

	codigo = recibir_operacion(cliente_fd);

	if(codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO){
					log_info(logger,"codigo de operacion incorrecto");
				}
	recibir_entero(cliente_fd);

	enviar_mensaje("Finalizar consola",cliente_fd,KERNEL_MENSAJE_FINALIZAR_CONSOLA);

	if(recibir_operacion(cliente_fd) == -1){
		log_error(logger, "La consola se desconecto. Finalizando Kernel");

	}



/*	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case KERNEL_PAQUETE_INSTRUCCIONES:
				listaInstrucciones = list_create();
				listaInstrucciones = recibir_paquete_instrucciones(cliente_fd);
				log_info(logger,"me llegaron las instrucciones");
				list_iterate(listaInstrucciones, (void*) iterator);
			break;
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

	  */


		PCB->idProceso = get_identificador();
		PCB->instrucciones = list_create();
		PCB->instrucciones = listaInstrucciones;
		PCB->program_counter = 0;
		inicializar_registros(PCB->registros);

		PCB->socket = cliente_fd;
		PCB->estado = NEW;
		PCB->algoritmoActual = RR;

		return PCB;
}

int conexionConCpu(t_pcb * PCB){
	log_info(logger,"creamos conexion con un puerto de cpu");
	int puertoInterrupt = crear_conexion(IP_KERNEL, "8001");
	log_info(logger,"creamos conexion con otro puerto de cpu");
	int puertoDispatch = crear_conexion(IP_KERNEL, "8002");
/*
	enviar_entero(10,puertoInterrupt,0);
	recibir_operacion(puertoInterrupt);
	recibir_entero(puertoInterrupt);

	enviar_entero(30,puertoDispatch,0);
	recibir_operacion(puertoDispatch);
	recibir_entero(puertoDispatch);
*/
	enviar_Pcb(PCB, 0, puertoDispatch);
	 return EXIT_SUCCESS;
}

/* Aca inicializamos una lista con elementos que contienen:
 * - dispositivo: nombre del dispositivo
 * - tiempo: tiempo del dispositivo
 * - cola_procesos: cola de los procesos que desean ejecutar ese dispositivo
 *
 * En caso de querer ejecutar un dispositivo, buscamos en lista_dispositivos el elemento
 * que matchee con el dispositivo deseado y desde ese elemento podemos obtener el tiempo
 * de uso y la disponibilidad del mismo con respecto a posibles otros procesos
 * que tambien desean usarlo.
 */
void inicializar_lista_dispositivos() {
	listaDeColasDispositivos = list_create();
	if (string_array_size(dispositivos_io) == string_array_size(tiempos_io)) {
		for (int i = 0; i < string_array_size(dispositivos_io); i++) {
			t_elem_disp* elemento_nuevo = malloc(sizeof(t_elem_disp));
			elemento_nuevo->dispositivo = dispositivos_io[i];
			elemento_nuevo->tiempo = atoi(tiempos_io[i]);
			elemento_nuevo->cola_procesos = queue_create();
			list_add(listaDeColasDispositivos,elemento_nuevo);
		}
	}
}

// En principio, no hace falta -------------------------------------------------------

// [disco,pantalla,teclado]
// [30,15,10]

/*
int obtener_indice_dispositivo(char* dispositivo) {
	int tamanio = sizeof(dispositivos_io); // / sizeof(dispositivos_io[0]);
	for (int i = 0; i < tamanio; i ++) {
//		char* dispositivo_aux = dispositivos_io[i];
		if (string_equals_ignore_case(dispositivos_io[i], dispositivo)) {
			return i;
		}
	}
	return -1;
}

int obtener_tiempo_dispositivo_io(char* dispositivo) {
	int indice = obtener_indice_dispositivo(dispositivo);
	if (indice == -1) {
		return -1;
	}
	return atoi(tiempos_io[indice]);
}
*/

// Retorna indice de lista en la que la cola de dispositivos fue agregada
//	en caso de que no se haya encontrado el indice solicitado por parametro

// IN PROGRESS - Esta mal inicialiazada
/*
int encolar_dispositivo(int indice, char* dispositivo) {
	t_queue* cola = list_get(colas_dispositivos, indice);
	if (queue_is_empty(cola) || cola == NULL) {
		t_list_disp* elemento_cola_dispositivo;
		t_queue* cola_nueva = queue_create();
		queue_push(cola_nueva, dispositivo);
		elemento_cola_dispositivo->cola = cola_nueva;
		elemento_cola_dispositivo->next = NULL;
		// Pasar esto a una funcion secundaria
		return list_add(colas_dispositivos, elemento_cola_dispositivo);
	}
	queue_push(cola, dispositivo);
	return 0;
}
*/
// En principio, no hace falta -------------------------------------------------------




void inicializar_semaforos(){
	/*

	sem_init(&desalojarProceso,0,0);
	sem_init(&procesoDesalojadoSem,0,1);
	sem_init(&pcbInterrupt,0,0);
	sem_init(&pcbBlocked,0,0);
	sem_init(&kernelSinFinalizar,0,0);

	pthread_mutex_init(&asignarMemoria,NULL);

	pthread_mutex_init(&ejecucion,NULL);
	pthread_mutex_init(&procesoExit,NULL);
	pthread_mutex_init(&consolasExit,NULL);
	pthread_mutex_init(&desalojandoProceso,NULL);
	pthread_mutex_init(&consolaNueva,NULL);
	pthread_mutex_init(&encolandoPcb,NULL);
	pthread_mutex_init(&mutexExit,NULL);
	pthread_mutex_init(&mutexInterrupt,NULL);
	pthread_mutex_init(&mutexIO,NULL);
	pthread_mutex_init(&bloqueandoProceso,NULL);
	*/

	sem_init(&pcbEnNew,0,0);
	sem_init(&pcbEnReady,0,0);
	sem_init(&cpuDisponible,0,1);

	sem_init(&gradoDeMultiprogramacion,0,gradoMultiprogramacionTotal);

	pthread_mutex_init(&mutexNew,NULL);
	pthread_mutex_init(&obtenerProceso,NULL);
}





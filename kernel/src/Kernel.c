#include <stdio.h>
#include <stdlib.h>
#include "funcionesKernel.h"

t_list* lista_dispositivos;

//void recibir_registros(int socket_cliente) // basado en recibir_operacion
//{
//	t_registros registros;
//	recv(socket_cliente, &registros, sizeof(t_registros), MSG_WAITALL);
//	log_info(logger,"Se recibio el registro AX de valor %d",registros.AX);
//	log_info(logger,"Se recibio el registro BX de valor %d",registros.BX);
//	log_info(logger,"Se recibio el registro CX de valor %d",registros.CX);
//	log_info(logger,"Se recibio el registro DX de valor %d",registros.DX);
//
//}
int conexionCpu = 0;

int main(int argc, char *argv[]) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}

	//sem_init(&kernelSinFinalizar,0,0);

	// Inicializaciones
	inicializar_configuraciones(argv[1]);
	//inicializar_listas_y_colas();

	log_info(logger,"Iniciando conexion con consola");

	conexionConConsola();
	log_info(logger, "me llegaron las instrucciones");
	list_iterate(listaInstrucciones, (void*) iterator);

	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->estado = READY;
	pcb->idProceso = 0;
	pcb->instrucciones = list_create();
	pcb->instrucciones = listaInstrucciones;
	pcb->programCounter = 0;
	pcb->registros.AX = 0;
	pcb->registros.BX = 0;
	pcb->registros.CX = 0;
	pcb->registros.DX = 0;
	pcb->tablaSegmentos = list_create();
	entradaTablaSegmento* unaEntrada = malloc(sizeof(entradaTablaSegmento));
	unaEntrada->numeroSegmento = 2;
	unaEntrada->numeroTablaPaginas = 0;
	unaEntrada->tamanioSegmento = 32;

	list_add(pcb->tablaSegmentos,unaEntrada);
	conexionCpu = crear_conexion(ipCpu, puertoCpuDispatch);

	agregar_a_paquete_kernel_cpu(pcb, KERNEL_PCB_A_CPU,conexionCpu);


	//free(unaEntrada);
	log_destroy(logger);

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
	gradoMultiprogramacionTotal = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION"); //me parece que no funcionan los get int
	quantum_rr = config_get_int_value(config,"QUANTUM_RR");
	dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
	tiempos_io = config_get_array_value(config, "TIEMPOS_IO");
}

void inicializar_listas_y_colas() {
	inicializar_lista_dispositivos();
}

int conexionConConsola(){
	static pthread_mutex_t mutexMensajes;

	int server_fd = iniciar_servidor(IP_KERNEL,PUERTO_KERNEL,"Consola");

	int cliente_fd = esperar_cliente(server_fd,"Consola");

	t_list *listaQueContieneTamSegmento;

	if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {
		listaQueContieneTamSegmento = list_create();
		listaQueContieneTamSegmento = recibir_lista_enteros(cliente_fd); //Hacer que reciba paquete vectorDeEnteros// Muy dificil mandar un vector dinamico, la lista la pueden usar como quieran
		// Y si usamos la funcion "atoi()"? //No entiendo para que, atoi te convierte string a entero
		int tamanioDelSegmento = (int) list_get(listaQueContieneTamSegmento, 3);
		log_info(logger, "El tamanio del primer segmento es: %d",
				tamanioDelSegmento);

		if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_INSTRUCCIONES) {
			listaInstrucciones = list_create();
			listaInstrucciones = recibir_paquete_instrucciones(cliente_fd);


			//enviar_mensaje("segmentos e instrucciones recibidos pibe", cliente_fd,KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS);
			//me parece que no haria falta con loguear que se recibieron ya alcanza
			pthread_mutex_lock(&mutexMensajes);
			//Esto es una prueba
			t_paquete* paquete = crear_paquete(KERNEL_PAQUETE_VALOR_A_IMPRIMIR);
			int valorAImprimirPrueba = 3;
			agregar_a_paquete_unInt(paquete, &valorAImprimirPrueba, sizeof(int));
			enviar_paquete(paquete, cliente_fd);
			eliminar_paquete(paquete);

			if(recibir_operacion(cliente_fd) == KERNEL_MENSAJE_VALOR_IMPRESO){
				recibir_mensaje(cliente_fd);
			}

			enviar_mensaje("Finalizar",cliente_fd,KERNEL_MENSAJE_FINALIZAR_CONSOLA);
			pthread_mutex_unlock(&mutexMensajes);

		} else {
			log_info(logger, "codigo de operacion incorrecto");
		}
	} else {
		log_info(logger, "codigo de operacion incorrecto");
	}


	if(recibir_operacion(cliente_fd) == -1){
		log_warning(logger, "La consola nro %d finalizo",cliente_fd); //kernel debe seguir a la espera de otras consolas
		return EXIT_SUCCESS;
	}


	//que problema hay con hacerlo asi? en la funcion de arriba se repite la logica del else codigo incorrecto!
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
	if (sizeof(dispositivos_io) == sizeof(tiempos_io)) {
		for (int i = 0; i < sizeof(dispositivos_io); i++) {
			t_elem_disp* elemento_nuevo = malloc(sizeof(t_elem_disp*));
			elemento_nuevo->dispositivo = dispositivos_io[i];
			elemento_nuevo->tiempo = atoi(tiempos_io[i]);
			elemento_nuevo->cola_procesos = queue_create();
			list_add(lista_dispositivos, elemento_nuevo);
		}
	}
}






#include "funcionesKernel.h"

t_list* lista_dispositivos;
char* dispositivosIOAplanado;
t_list *listaTamanioSegmentos;

//t_list* listaDeColasDispositivos;

int conexionCpu = 0;
int conexionCpuInterrupt = 0;


int main(int argc, char *argv[]) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}


	//sem_init(&kernelSinFinalizar,0,0);

	// Inicializaciones

	inicializar_configuraciones(argv[1]);

	dispositivosIOAplanado = string_new();
	aplanarDispositivosIO(dispositivos_io);

	log_info(logger,"DISPOSITIVOS: %s",dispositivosIOAplanado);
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
	unaEntrada->tamanioSegmento = 32;//para probar, esto hay que inicializarlo con: nroTabla -> viene de memoria
																				// nroSegmento -> viene de memoria(o incremental)
																				// tamanio -> viene del config de consola(lista)
	list_add(pcb->tablaSegmentos,unaEntrada);
	conexionCpu = crear_conexion(ipCpu, puertoCpuDispatch);
	sleep(1);
	conexionCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	log_info(logger,"Conexion con interrupt establecida");

	agregar_a_paquete_kernel_cpu(pcb, KERNEL_PCB_A_CPU,conexionCpu);
	//free(unaEntrada);

	enviar_mensaje(dispositivosIOAplanado, conexionCpu, KERNEL_MENSAJE_DISPOSITIVOS_IO);


	enviar_mensaje("DESALOJO",conexionCpuInterrupt,KERNEL_MENSAJE_INTERRUPT); //Para probar

	log_info(logger,"Interrupcion enviada");
/*
	while(1){

	}

*/
	log_destroy(logger);

	//inicializar_configuraciones(argv[1]);
	identificadores_pcb = 0;


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
	pthread_t hiloAdmin[3];
	int hiloAdminCreado[3];


	//conexiones
	socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	socketCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);
	socketCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	socketServidor = iniciar_servidor();

	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,socketServidor);
	pthread_detach(hiloCreado);

	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
	hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&atender_interrupcion_de_ejecucion,NULL); // problemas con esto
	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&readyAExe,NULL);


	pthread_detach(hiloAdmin[0]);
	pthread_detach(hiloAdmin[1]);
	pthread_detach(hiloAdmin[2]);

	list_iterate(listaDeColasDispositivos,(void *)levantar_hilo_dispositivo);

*/


	log_destroy(logger);
	//sem_wait(&kernelSinFinalizar);

	//int nroTabla1erNivel = conexionConMemoria();


	//cargar_pcb(nroTabla1erNivel);

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

	colaNew = queue_create();
	colaReadyFIFO = queue_create();
	colaReadyRR = queue_create();


}




t_pcb* conexionConConsola() {

static pthread_mutex_t mutexMensajes;
int server_fd = iniciar_servidor(IP_KERNEL, PUERTO_KERNEL, "Consola");

int cliente_fd = esperar_cliente(server_fd, "Consola");

if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {
	listaTamanioSegmentos = list_create();
	listaTamanioSegmentos = recibir_lista_enteros(cliente_fd); //Hacer que reciba paquete vectorDeEnteros// Muy dificil mandar un vector dinamico, la lista la pueden usar como quieran
	// Y si usamos la funcion "atoi()"? //No entiendo para que, atoi te convierte string a entero
	int tamanioDelSegmento = (int) list_get(listaTamanioSegmentos, 0);
	log_info(logger, "El tamanio del primer segmento es: %d",
			tamanioDelSegmento);

	t_list *listaQueContieneTamSegmento;
	t_pcb *PCB = malloc(sizeof(t_pcb*));

	log_info(logger, "llego antes de recibir_operacion");

	int operacion = recibir_operacion(cliente_fd);

	log_info(logger, "se recibio la operacion %d", operacion);

	if (operacion == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {
		//	listaQueContieneTamSegmento = list_create();
		//	listaQueContieneTamSegmento = recibir_lista_enteros(cliente_fd);
		PCB->tablaSegmentos = list_create();
		PCB->tablaSegmentos = inicializar_tabla_segmentos(cliente_fd); // aca usariamos el recibir_lista_enteros
		//	int tamanioDelSegmento = (int) list_get(listaQueContieneTamSegmento,3);
		//			log_info(logger,"El tamanio del primer segmento es: %d",tamanioDelSegmento);

		enviar_mensaje(dispositivosIOAplanado, cliente_fd,
				KERNEL_MENSAJE_DISPOSITIVOS_IO);

		if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_INSTRUCCIONES) {
			listaInstrucciones = list_create();
			listaInstrucciones = recibir_paquete_instrucciones(cliente_fd);

			//enviar_mensaje("segmentos e instrucciones recibidos pibe", cliente_fd,KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS);
			//me parece que no haria falta con loguear que se recibieron ya alcanza
			pthread_mutex_lock(&mutexMensajes);
			//Esto es una prueba
			enviar_entero(3, cliente_fd, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

			if (recibir_operacion(cliente_fd) == KERNEL_MENSAJE_VALOR_IMPRESO) {
				recibir_mensaje(cliente_fd);
			}

			enviar_mensaje("Finalizar", cliente_fd,
					KERNEL_MENSAJE_FINALIZAR_CONSOLA);
			pthread_mutex_unlock(&mutexMensajes);

		} else {
			log_info(logger, "codigo de operacion incorrecto");
		}
	} else {
		log_info(logger, "codigo de operacion incorrecto");
	}

	enviar_entero(10, cliente_fd, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

	int codigo = recibir_operacion(cliente_fd);
	if (codigo != KERNEL_MENSAJE_VALOR_IMPRESO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	recibir_mensaje(cliente_fd);

	enviar_mensaje("kernel solicita que se ingrese un valor por teclado",
			cliente_fd, KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO);

	codigo = recibir_operacion(cliente_fd);
	if (codigo != KERNEL_MENSAJE_DESBLOQUEO_TECLADO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	recibir_mensaje(cliente_fd);

	codigo = recibir_operacion(cliente_fd);

	if (codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	recibir_entero(cliente_fd);

	enviar_mensaje("Finalizar consola", cliente_fd,
			KERNEL_MENSAJE_FINALIZAR_CONSOLA);

	if (recibir_operacion(cliente_fd) == -1) {

		log_warning(logger, "La consola nro %d finalizo", cliente_fd); //kernel debe seguir a la espera de otras consolas
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

	PCB->idProceso = get_identificador();
	PCB->instrucciones = list_create();
	PCB->instrucciones = listaInstrucciones;
	PCB->programCounter = 0;
	inicializar_registros(PCB->registros);

	PCB->socket = cliente_fd;
	PCB->estado = NEW;
	PCB->algoritmoActual = RR;

	return PCB;
}
}

//int conexionConCpu(t_pcb * PCB){
//	log_info(logger,"creamos conexion con un puerto de cpu");
//	int puertoInterrupt = crear_conexion(IP_KERNEL, "8001");
//	log_info(logger,"creamos conexion con otro puerto de cpu");
//	int puertoDispatch = crear_conexion(IP_KERNEL, "8002");
///*
//	enviar_entero(10,puertoInterrupt,0);
//	recibir_operacion(puertoInterrupt);
//	recibir_entero(puertoInterrupt);
//
//	enviar_entero(30,puertoDispatch,0);
//	recibir_operacion(puertoDispatch);
//	recibir_entero(puertoDispatch);
//*/
//	agregar_a_paquete_kernel_cpu(PCB, 0, puertoDispatch);
//	 return EXIT_SUCCESS;
//}

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
			elemento_nuevo->cola_UTs = queue_create();

			sem_t semaforo;
			sem_init(&semaforo,0,0);
			elemento_nuevo->semaforo = semaforo;


			list_add(listaDeColasDispositivos,elemento_nuevo);
		}
	}
}

void aplanarDispositivosIO(char** dispositivos_io){

	for(int i = 0; i < string_array_size(dispositivos_io); i++){
		string_append(&dispositivosIOAplanado,dispositivos_io[i]);

		if(i < string_array_size(dispositivos_io)-1){
			string_append(&dispositivosIOAplanado,"-");
		}

	}

}


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
	pthread_mutex_init(&mutexPantalla,NULL);
	pthread_mutex_init(&mutexTeclado,NULL);
}





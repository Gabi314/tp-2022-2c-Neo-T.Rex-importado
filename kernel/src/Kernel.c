#include "funcionesKernel.h"

t_list* lista_dispositivos;
t_list *listaTamanioSegmentos;

//t_list* listaDeColasDispositivos;
/*
int conexionCpu = 0;
int conexionCpuInterrupt = 0;
int socketMemoria = 0;
*/

int main(int argc, char *argv[]) {

//-----------------------------------------------PRIMER PARTE DEL MAIN ------------------------------------------------------------

	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	loggerAux = log_create("kernelAux.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}


	//sem_init(&kernelSinFinalizar,0,0); // puede que lo usemos algunas veces

	// Inicializacion de kernel como servidor
	socketServidorKernel = iniciar_servidor(IP_KERNEL, PUERTO_KERNEL, "Consola");
	log_info(logger,"servidor kernel inicializado");

	inicializar_configuraciones(argv[1]);

	// armamos el string de dispositivos que enviamos a consola
	dispositivosIOAplanado = string_new();
	aplanarDispositivosIO(dispositivos_io);
	log_info(logger,"DISPOSITIVOS: %s",dispositivosIOAplanado);

	socketMemoria = crear_conexion(ipMemoria, puertoMemoria);
	conexionCpuDispatch = crear_conexion(ipCpu, puertoCpuDispatch);

	enviar_mensaje(dispositivosIOAplanado, conexionCpuDispatch,
						KERNEL_MENSAJE_DISPOSITIVOS_IO);
	conexionCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	// Inicializaciones de estructuras de datos

	log_info(logger,"configuraciones inicializadas");
	inicializar_listas_y_colas();
	log_info(logger,"listas y colas inicializadas");
	identificadores_pcb = 0;
	inicializar_semaforos();
	log_info(logger,"semaforos inicializados");
	log_info(logger, "En la lista de dispositivos tenemos los siguientes:");
		for (int i = 0; i < list_size(listaDeColasDispositivos); i++) {
			t_elem_disp* aux = malloc(sizeof(t_elem_disp));
			aux = list_get(listaDeColasDispositivos,i);
			log_info(logger,"el dispositivo de posicion %d se llama %s", i , aux->dispositivo);
			log_info(logger,"el dispositivo de posicion %d tiene un tiempo de %d", i, aux->tiempo);
				}




// ------------------------------------------------- PRUEBAS DE GABI DE CONEXIONES -----------------------------------------------------
/*
	// prueba de conexion con la consola
	log_info(logger,"iniciamos prueba de conexion con consola");
	pruebaDeConexionConConsola();

	//t_pcb * PCB = pruebaDeConexionConConsola();
	//pruebaDeConexionConCpu(PCB);


	log_destroy(logger);
*/
// -------------------------------------------------- PRUEBAS DE LEO DE CONEXIONES ---------------------------------------------------

/*
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

	conexionCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	log_info(logger,"Conexion con interrupt establecida");

	agregar_a_paquete_kernel_cpu(pcb, KERNEL_PCB_A_CPU,conexionCpu);
	//free(unaEntrada);

	enviar_mensaje(dispositivosIOAplanado, conexionCpu, KERNEL_MENSAJE_DISPOSITIVOS_IO);


	enviar_mensaje("DESALOJO",conexionCpuInterrupt,KERNEL_MENSAJE_INTERRUPT); //Para probar

	log_info(logger,"Interrupcion enviada");

	log_destroy(logger);

	log_destroy(loggerAux);
*/
//------------------------------------------------SEGUNDA PARTE DEL MAIN ----------------------------------------------------------

	pthread_t hilo0;
	pthread_t hiloAdmin[3];
	int hiloAdminCreado[3];


	//conexiones



	int hiloCreado = pthread_create(&hilo0, NULL,&recibir_consola,&socketServidorKernel);
	pthread_detach(hilo0);

	hiloAdminCreado[0] = pthread_create(&hiloAdmin[0],NULL,&asignar_memoria,NULL);
	hiloAdminCreado[1] = pthread_create(&hiloAdmin[1],NULL,&atender_interrupcion_de_ejecucion,NULL); // problemas con esto
	hiloAdminCreado[2] = pthread_create(&hiloAdmin[2],NULL,&readyAExe,NULL);


	pthread_detach(hiloAdmin[0]);
	pthread_detach(hiloAdmin[1]);
	pthread_detach(hiloAdmin[2]);

	list_iterate(listaDeColasDispositivos,(void *)levantar_hilo_dispositivo);



//-------------------------------------------------RESTOS DE OTRAS PARTES -----------------------------------------------------------

	sem_wait(&kernelSinFinalizar);

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



/*
t_pcb* pruebaDeConexionConConsola() {

log_info(logger,"esperando a la consola");
int cliente_fd = esperar_cliente(socketServidorKernel, "Consola");
log_info(logger,"es la consola numero %d",cliente_fd);

if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {
	listaTamanioSegmentos = list_create();
	listaTamanioSegmentos = recibir_lista_enteros(cliente_fd); //Hacer que reciba paquete vectorDeEnteros// Muy dificil mandar un vector dinamico, la lista la pueden usar como quieran
	// Y si usamos la funcion "atoi()"? //No entiendo para que, atoi te convierte string a entero
	int tamanioDelSegmento = (int) list_get(listaTamanioSegmentos, 0);
	log_info(logger, "El tamanio del primer segmento es: %d",
			tamanioDelSegmento);
*/
	/*
	t_list *listaQueContieneTamSegmento;
	t_pcb *PCB = malloc(sizeof(t_pcb*));


// SE RECIBEN LOS SEGMENTOS Y SE METEN EN EL PCB

	log_info(logger,"esperando segmentos");
	int operacion = recibir_operacion(cliente_fd);



	if (operacion == KERNEL_PAQUETE_TAMANIOS_SEGMENTOS) {

		PCB->tablaSegmentos = list_create();
		PCB->tablaSegmentos = inicializar_tabla_segmentos(cliente_fd); // aca usariamos el recibir_lista_enteros


//SE ENVIAN LOS DISPOSITIVOS
		log_info(logger,"enviamos los dispositivos");
		enviar_mensaje(dispositivosIOAplanado, cliente_fd,
				KERNEL_MENSAJE_DISPOSITIVOS_IO);

//SE RECIBEN LAS INSTRUCCIONES
		log_info(logger,"esperando instrucciones");
		if (recibir_operacion(cliente_fd) == KERNEL_PAQUETE_INSTRUCCIONES) {
			listaInstrucciones = list_create();
			listaInstrucciones = recibir_paquete_instrucciones(cliente_fd);

//MOSTRAMOS LAS INSTRUCCIONES
			log_info(logger, "me llegaron las instrucciones");
			list_iterate(listaInstrucciones, (void*) iteratorMostrarInstrucciones);

//METEMOS LAS INSTRUCCIONES EN EL PCB

			PCB->instrucciones = list_create();
			PCB->instrucciones = listaInstrucciones;
			log_info(logger,"metemos instrucciones en el pcb");

// CONFIRMAMOS RECEPCION DE INSTRUCCIONES
			enviar_mensaje("segmentos e instrucciones recibidos pibe", cliente_fd,KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS);

*/
		/*
			pthread_mutex_lock(&mutexMensajes);
			//Esto es una prueba
			enviar_entero(3, cliente_fd, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);

			if (recibir_operacion(cliente_fd) == KERNEL_MENSAJE_VALOR_IMPRESO) {
				recibir_mensaje(cliente_fd);
			}

			enviar_mensaje("Finalizar", cliente_fd,
					KERNEL_MENSAJE_FINALIZAR_CONSOLA);
			pthread_mutex_unlock(&mutexMensajes);
*/
	/*
		} else {
			log_info(logger, "codigo de operacion incorrecto");
		}
	} else {
		log_info(logger, "codigo de operacion incorrecto");
	}


//ENVIAMOS UN ENTERO PARA QUE SE IMPRIMA POR PANTALLA
	log_info(logger,"enviamos un entero a la consola");
	enviar_entero(10, cliente_fd, KERNEL_PAQUETE_VALOR_A_IMPRIMIR);


//RECIBIMOS CONFIRMACION DE QUE EL VALOR FUE IMPRESO

	int codigo = recibir_operacion(cliente_fd);
	if (codigo != KERNEL_MENSAJE_VALOR_IMPRESO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	recibir_mensaje(cliente_fd);

//ENVIAMOS UN PEDIDO DE QUE SE INGRESE UN VALOR POR TECLADO
	log_info(logger,"enviamos un pedido de ingresar un valor por teclado");
	enviar_mensaje("kernel solicita que se ingrese un valor por teclado",
			cliente_fd, KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO);


//RECIBIMOS EL VALOR POR TECLADO
	codigo = recibir_operacion(cliente_fd);
	if (codigo != KERNEL_MENSAJE_DESBLOQUEO_TECLADO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	recibir_mensaje(cliente_fd);

	codigo = recibir_operacion(cliente_fd);

	if (codigo != KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO) {
		log_info(logger, "codigo de operacion incorrecto");
	}
	log_info(logger,"esperamos el valor ingresado por teclado");
	recibir_entero(cliente_fd);


// FINALIZACION DE CONSOLA
	log_info(logger,"enviamos un mensaje a consola para que finalice");
	enviar_mensaje("Finalizar consola", cliente_fd,
			KERNEL_MENSAJE_FINALIZAR_CONSOLA);

	if (recibir_operacion(cliente_fd) == -1) {

		log_warning(logger, "La consola nro %d finalizo", cliente_fd); //kernel debe seguir a la espera de otras consolas
		return EXIT_SUCCESS;

	}




	PCB->idProceso = get_identificador();

	PCB->programCounter = 0;
	inicializar_registros(PCB->registros);

	PCB->socket = cliente_fd;
	PCB->estado = NEW;
	PCB->algoritmoActual = RR;

	return PCB;
}
*/

//int pruebaDeConexionConCpu(t_pcb * PCB){
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
	pthread_mutex_init(&primerPushColaReady,NULL);
	pthread_mutex_init(&mutexPantalla,NULL);
	pthread_mutex_init(&mutexTeclado,NULL);
}





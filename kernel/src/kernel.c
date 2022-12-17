#include "funcionesKernel.h"

t_list* lista_dispositivos;
t_list *listaTamanioSegmentos;

int main(int argc, char *argv[]) {

	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);

	loggerAux = log_create("kernelAux.log", "KERNEL", 1, LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}

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

	conexionCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt);
	enviar_mensaje(dispositivosIOAplanado, conexionCpuDispatch,
						KERNEL_MENSAJE_DISPOSITIVOS_IO);

	// Inicializaciones de estructuras de datos

	log_info(logger,"configuraciones inicializadas");
	inicializar_listas_y_colas();
	log_info(logger,"listas y colas inicializadas");
	identificadores_pcb = 0;
	inicializar_semaforos();
	log_info(logger,"semaforos inicializados");
	log_info(logger, "En la lista de dispositivos tenemos los siguientes:");
	for (int i = 0; i < list_size(listaDeColasDispositivos); i++) {
		t_elem_disp* aux;
		aux = list_get(listaDeColasDispositivos,i);
		log_info(logger,"el dispositivo de posicion %d se llama %s", i , aux->dispositivo);
		log_info(logger,"el dispositivo de posicion %d tiene un tiempo de %d", i, aux->tiempo);
	}

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

	sem_wait(&kernelSinFinalizar);

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

			pthread_mutex_t mutex;
			pthread_mutex_t mutex_cola_procesos;
			pthread_mutex_init(&mutex,NULL);
			pthread_mutex_init(&mutex_cola_procesos,NULL);
			elemento_nuevo->mutexDisp = mutex;
			elemento_nuevo->mutex_cola_procesos = mutex_cola_procesos;

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
	sem_init(&pruebaIOGen,0,0);

	sem_init(&pcbEnNew,0,0);
	sem_init(&pcbEnReady,0,0);
	sem_init(&cpuDisponible,0,1);

	sem_init(&gradoDeMultiprogramacion,0,gradoMultiprogramacionTotal);

	pthread_mutex_init(&mutexNew,NULL);
	pthread_mutex_init(&obtenerProceso,NULL);
	pthread_mutex_init(&primerPushColaReady,NULL);
	pthread_mutex_init(&mutexPantalla,NULL);
	pthread_mutex_init(&mutexTeclado,NULL);
	pthread_mutex_init(&mutexConexionMemoria,NULL);

	pthread_mutex_init(&mutex_cola_ready_FIFO, NULL);
	pthread_mutex_init(&mutex_cola_ready_RR, NULL);
}





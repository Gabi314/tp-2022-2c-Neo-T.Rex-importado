#include "funcionesCpu.h"
#include "tlb.h"

//------------------DECLARO VARIABLES
t_log* logger;
t_log* loggerObligatorio;
t_config* config;

int cantidadEntradasTlb;
char* algoritmoReemplazoTlb;
int retardoDeInstruccion;

int tamanioDePagina;
int entradasPorTabla;

int numeroDeSegmento;
int desplazamientoDeSegmento;
int tamanioMaximoDeSegmento;
int numeroDePagina;
int desplazamientoDePagina;

uint32_t ax;
uint32_t bx;
uint32_t cx;
uint32_t dx;

bool hayInterrupcion = false;

pthread_t hiloInterrupciones;
pthread_mutex_t mutexInterrupcion;

sem_t pcbRecibido;


//--------------------FUNCIONES ELEMENTALES------------------------------
int chequeoCantidadArchivos(int argc){
	if(argc < 2) {
		    log_error(logger,"Falta un parametro");
		    return EXIT_FAILURE;
		}
	return EXIT_SUCCESS;
}

void inicializarConfiguraciones(char* unaConfig){
	config = config_create(unaConfig);// ver bien como recibir los path de config por parametros

	ipMemoria = config_get_string_value(config,"IP_MEMORIA");
	puertoMemoria = config_get_string_value(config,"PUERTO_MEMORIA");

	cantidadEntradasTlb = config_get_int_value(config,"ENTRADAS_TLB");
	algoritmoReemplazoTlb = config_get_string_value(config,"REEMPLAZO_TLB");
	retardoDeInstruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");

	puertoDeEscuchaDispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
	puertoDeEscuchaInterrupt = config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");

	sem_init(&pcbRecibido,0,0);
	pthread_mutex_init(&mutexEjecutar, NULL);
	pthread_mutex_init(&mutexInterrupcion, NULL);
}


instruccion* buscarInstruccionAEjecutar(t_pcb* unPCB){//FETCH CREO QUE ES IGUAL
	if((unPCB->programCounter)<list_size(unPCB->instrucciones)){
	instruccion* unaInstruccion = list_get(unPCB->instrucciones,unPCB->programCounter);
	//unPCB->programCounter += 1;

	return unaInstruccion;
	}
}

int decode(instruccion* unaInstruccion){
	if(! strcmp(unaInstruccion->identificador,"SET")){
		return SET;
	}else if(! strcmp(unaInstruccion->identificador,"ADD")){
		return ADD;
	}else if(! strcmp(unaInstruccion->identificador,"MOV_IN")){
		return MOV_IN;
	}else if(! strcmp(unaInstruccion->identificador,"MOV_OUT")){
		return MOV_OUT;
	}else if(! strcmp(unaInstruccion->identificador,"I/O")){
		return IO;
	}else if(! strcmp(unaInstruccion->identificador,"EXIT")){
		return EXT;
	}
	return -1;
}

uint32_t registroAUtilizar(int registroInstruccion,t_registros registroPcb){
	if(registroInstruccion == AX){
		return registroPcb.AX;
	}
	if(registroInstruccion == BX){
		return registroPcb.BX;
	}
	if(registroInstruccion == CX){
		return registroPcb.CX;
	}
	if(registroInstruccion == DX){
		return registroPcb.DX;
	}

	return -1;
}

void modificarRegistro(int valor,int registroInstruccion,t_pcb* unPcb){
	if(registroInstruccion == AX){
		 unPcb->registros.AX = valor;
	}
	if(registroInstruccion == BX){
		unPcb->registros.BX = valor;
	}
	if(registroInstruccion == CX){
		unPcb->registros.CX = valor;
	}
	if(registroInstruccion == DX){
		unPcb->registros.DX = valor;
	}
}

char* imprimirRegistro(registros unRegistro){//poner en shared si sirve para kernel
	if(unRegistro == AX){
		return "AX";
	}
	if(unRegistro == BX){
		return "BX";
	}
	if(unRegistro == CX){
		return "DX";
	}
	if(unRegistro == DX){
		return "CX";
	}
	return NULL;
}

char* dispositivoIOSegunParametro(int parametro){
	if(parametro == PANTALLA)
	return "PANTALLA";
	if(parametro == TECLADO)
	return "TECLADO";

	for(int i=0; i < string_array_size(listaDispositivos); i++){
		if(i+2 == parametro){//sumo 2 porque la posicion 0 y 1 pertence a pantalla y teclado
			return listaDispositivos[i];
		}
	}

	return NULL;
}

void ejecutar(instruccion* unaInstruccion,t_pcb* pcb){
	int primerParametro = unaInstruccion->parametros[0];
	int segundoParametro = unaInstruccion->parametros[1];
	int direccionLogica;
	int marco; //Warning porque no se usa en todos los case
	uint32_t registro;
	uint32_t segundoRegistro;

	if(!hayInterrupcion){
		switch(decode(unaInstruccion)){
			case SET:
				log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
						pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
							segundoParametro);
				usleep(retardoDeInstruccion);

				modificarRegistro((uint32_t) segundoParametro,primerParametro,pcb); //En set el primer parametro es el registro
				log_info(logger,"Valor del registro: %u",pcb->registros.AX);
				//En set el segundo registro es el valor a asignar
				log_info(logger,"----------------FINALIZA SET----------------\n");
				pcb->programCounter += 1;
				break;
			case ADD:
				log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%s>",
								pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
									imprimirRegistro(segundoParametro));
				usleep(retardoDeInstruccion);

				registro = registroAUtilizar(primerParametro,pcb->registros);
				segundoRegistro = registroAUtilizar(segundoParametro,pcb->registros);
				registro += segundoRegistro;

				modificarRegistro(registro,primerParametro,pcb);
				log_info(logger,"----------------FINALIZA ADD----------------\n");
				pcb->programCounter += 1;
				break;
			case MOV_IN:
				log_info(logger,"----------------EXECUTE MOV_IN----------------");
				log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
								pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
									segundoParametro);
				registro = registroAUtilizar(primerParametro,pcb->registros);
				direccionLogica = segundoParametro;

				if(calculoDireccionLogicaExitoso(direccionLogica,pcb->tablaSegmentos)){
					marco = buscarDireccionFisica(pcb);
					if(marco == -1){// Hay PF
						bloqueoPorPageFault(pcb);
						break;
					}
					log_info(logger,"No hizo el break por page fault!!!!");
					log_info(loggerObligatorio,
						"PID: <%d> - Acción: <LEER> - Segmento: <%d> - Pagina: <%d> - Dirección Fisica: <%d>"
						,pcb->idProceso
						,numeroDeSegmento
						,numeroDePagina
						,(marco*tamanioDePagina + desplazamientoDePagina));

					enviarDireccionFisica(marco,desplazamientoDePagina,1,-1); //1 valor a leer de memoria y registro -1 porque aca no hay que enviar uno

					uint32_t valorAAlmacenar;

					int cod_op = recibir_operacion(socket_memoria);

					if(cod_op == MEMORIA_A_CPU_NUMERO_LEIDO){// Recibe el valor leido de memoria
						valorAAlmacenar = recibir_entero(socket_memoria);
						modificarRegistro(valorAAlmacenar,primerParametro,pcb);
						log_info(logger,"----------------FINALIZA MOV_IN----------------\n");
					}
					 // y se almacena en el registro(primer parametro)
				}else{
					log_info(logger,"Error: Segmentation Fault (segmento nro: %d)",numeroDeSegmento);//Enviar este mensaje a kernel y devolver pcb
					enviar_pcb(pcb, CPU_PCB_A_KERNEL_PCB_POR_FINALIZACION, clienteKernel);
					pthread_mutex_lock(&mutexEjecutar);
					ejecutando = false;
					pthread_mutex_unlock(&mutexEjecutar);
					log_info(logger,"----------------FINALIZA MOV_IN----------------\n");
				}

				pcb->programCounter += 1;
				break;
			case MOV_OUT:
				log_info(logger,"----------------EXECUTE MOV_OUT----------------");
				log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s> - <%d> - <%s>",
												pcb->idProceso,unaInstruccion->identificador,primerParametro,
												imprimirRegistro(segundoParametro));
				direccionLogica = primerParametro;

				registro = registroAUtilizar(segundoParametro,pcb->registros);//leo el valor del registro primer parametro

				if(calculoDireccionLogicaExitoso(direccionLogica,pcb->tablaSegmentos)){
					marco = buscarDireccionFisica(pcb);
					if(marco == -1){// Hay PF
						bloqueoPorPageFault(pcb);
						break;
					}
					log_info(logger,"No salio!!!!");
					log_info(loggerObligatorio,
						"PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Pagina: <%d> - Dirección Fisica: <%d>"
						,pcb->idProceso
						,numeroDeSegmento
						,numeroDePagina
						,(marco*tamanioDePagina + desplazamientoDePagina));
					enviarDireccionFisica(marco,desplazamientoDePagina,0,registro);//con 0 envia la dir fisica para escribir

					int cod_op = recibir_operacion(socket_memoria);

					if(cod_op == MENSAJE_CPU_MEMORIA){// Recibe que se escribio correctamente el valor en memoria
						recibir_mensaje(socket_memoria);
						log_info(logger,"----------------FINALIZA MOV_OUT----------------\n");
					}
				}else{
					log_info(logger,"Error: Segmentation Fault (segmento nro: %d)",numeroDeSegmento);//Enviar este mensaje a kernel y devolver pcb
					enviar_pcb(pcb, CPU_PCB_A_KERNEL_PCB_POR_FINALIZACION, clienteKernel);
					pthread_mutex_lock(&mutexEjecutar);
					ejecutando = false;
					pthread_mutex_unlock(&mutexEjecutar);
					log_info(logger,"----------------FINALIZA MOV_IN----------------\n");
				}
				pcb->programCounter += 1;
				break;
			case IO:
				if(primerParametro != TECLADO && primerParametro != PANTALLA){
					log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
							pcb->idProceso, unaInstruccion->identificador,
							dispositivoIOSegunParametro(primerParametro),
							segundoParametro);
					pcb->programCounter += 1;
					enviar_pcb(pcb, CPU_PCB_A_KERNEL_POR_IO, clienteKernel);//envio pcb por IO general

					enviar_mensaje(dispositivoIOSegunParametro(primerParametro),clienteKernel,CPU_DISPOSITIVO_A_KERNEL);

					enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_UNIDADES_DE_TRABAJO_IO);
				} else {
					log_info(loggerObligatorio,"PID: <%d> - Ejecutando: <%s> - <%s> - <%s>",
							pcb->idProceso, unaInstruccion->identificador,
							dispositivoIOSegunParametro(primerParametro),
							imprimirRegistro(segundoParametro));

					if(primerParametro == TECLADO){
						pcb->programCounter += 1;
						enviar_pcb(pcb, CPU_PCB_A_KERNEL_POR_IO_TECLADO, clienteKernel);
						enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_INGRESAR_VALOR_POR_TECLADO);
					}else if(primerParametro == PANTALLA){
						pcb->programCounter += 1;
						enviar_pcb(pcb, CPU_PCB_A_KERNEL_POR_IO_PANTALLA, clienteKernel);
						enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_MOSTRAR_REGISTRO_POR_PANTALLA);
					}
				}
				ejecutando = false;
				log_info(logger,"----------------FINALIZA I/O----------------\n");

				break;
			case EXT:
				log_info(loggerObligatorio,"“PID: <%d> - Ejecutando: <%s>",pcb->idProceso,unaInstruccion->identificador);
				pthread_mutex_lock(&mutexEjecutar);
				ejecutando = false;
				pthread_mutex_unlock(&mutexEjecutar);
				limpiarEntradasTLB(pcb->idProceso);
				enviar_pcb(pcb, CPU_PCB_A_KERNEL_PCB_POR_FINALIZACION, clienteKernel);
				log_info(logger,"----------------FINALIZA EXIT----------------\n");
				break;
		}
	}else{
		log_info(logger,"Interrupcion!!!");
		pthread_mutex_lock(&mutexEjecutar);
		ejecutando = false;
		pthread_mutex_unlock(&mutexEjecutar);
		enviar_pcb(pcb, CPU_A_KERNEL_PCB_POR_DESALOJO, clienteKernel);
		//hayInterrupcion = false; ?
	}
}

void checkInterrupt(){

	pthread_create(&hiloInterrupciones, NULL, (void*) escucharInterrupciones,
			NULL);

	pthread_detach(hiloInterrupciones);
}

void escucharInterrupciones(){
	log_info(logger, "Cpu escuchando interrupciones");
		while (1) {
			int cod_op = recibir_operacion(clienteKernelInterrupt);
			log_info(logger, "Entro");
			if (cod_op == DESALOJAR_PROCESO_POR_FIN_DE_QUANTUM) {
				pthread_mutex_lock(&mutexInterrupcion);
				recibir_mensaje(clienteKernelInterrupt);
				hayInterrupcion = true;
				pthread_mutex_unlock(&mutexInterrupcion);
				//return EXIT_SUCCESS;
			} else if (cod_op == -1) {
				log_info(logger, "Se desconecto el kernel. Terminando conexion");
				sem_t spreenMiCasita;
				sem_init(&spreenMiCasita,0,0);
				sem_wait(&spreenMiCasita);
				//return EXIT_SUCCESS;
			}
		}
		//return EXIT_SUCCESS;
}

bool calculoDireccionLogicaExitoso(int direccionLogica,t_list* listaTablaSegmentos){
	tamanioMaximoDeSegmento = entradasPorTabla * tamanioDePagina;
	numeroDeSegmento = floor(direccionLogica / tamanioMaximoDeSegmento);
	desplazamientoDeSegmento = direccionLogica % tamanioMaximoDeSegmento;
	numeroDePagina = floor(desplazamientoDeSegmento  / tamanioDePagina);
	desplazamientoDePagina = desplazamientoDeSegmento % tamanioDePagina;

	return !haySegFault(numeroDeSegmento,desplazamientoDeSegmento,listaTablaSegmentos);
}

int buscarDireccionFisica(t_pcb* pcb){
	int marco = chequearMarcoEnTLB(numeroDePagina,numeroDeSegmento,pcb->idProceso);

	if (marco == -1){
		return accederAMemoria(marco,numeroDeSegmento,pcb);
	}

	//usleep(retardoDeInstruccion);
	return marco;
}

int chequearMarcoEnTLB(int nroDePagina, int nroDeSegmento,int pid){
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
	int marco = -1;

	for(int i=0;i < list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);

		if (unaEntradaTLB->nroDePagina == nroDePagina
				&& unaEntradaTLB->nroDeSegmento == nroDeSegmento
				&& unaEntradaTLB->nroDeProceso == pid) {

			unaEntradaTLB->ultimaReferencia = time(NULL);// se referencio a esa pagina
			marco = unaEntradaTLB->nroDeMarco;
			log_info(loggerObligatorio,
					"PID: <%d> - TLB HIT - Segmento: <%d> - Pagina: <%d>", pid,
					nroDeSegmento, nroDePagina);

			free(unaEntradaTLB);
			return marco; // se puede almacenar este valor en una variable y retornarlo fuera del for
		}
	}
	free(unaEntradaTLB);
	log_info(loggerObligatorio,
			"PID: <%d> - TLB MISS - Segmento: <%d> - Pagina: <%d>", pid,
			nroDeSegmento, nroDePagina);
	return marco;
}

bool haySegFault(int numeroDeSegmento, int desplazamientoSegmento,t_list * listaDeTablaSegmentos){
	entradaTablaSegmento* unaEntradaDeTabla;
	for(int i; i< list_size(listaDeTablaSegmentos);i++){
		unaEntradaDeTabla = list_get(listaDeTablaSegmentos,i);
		if(numeroDeSegmento == unaEntradaDeTabla->numeroSegmento){
			if(desplazamientoSegmento > unaEntradaDeTabla->tamanioSegmento){
				return true;//hay SF->>>>> devolverse el proceso al Kernel para que este lo finalice con motivo de Error: Segmentation Fault
			}
		}
	}

	return false;
}


int accederAMemoria(int marco,int numeroDeSegmento,t_pcb* pcb){

	log_info(logger,"La pagina no se encuentra en tlb, se debera acceder a memoria(tabla de paginas)");
	enviarNroTablaDePaginas(pcb->tablaSegmentos,numeroDeSegmento,socket_memoria,numeroDePagina);//Envio nroTP y nroDePag(los obtengo con nro de segmento de la tabla de segmentos)

	int seAccedeAMemoria = 1;

	while (seAccedeAMemoria == 1) {
		int cod_op = recibir_operacion(socket_memoria);

		switch (cod_op){
			case MEMORIA_A_CPU_NUMERO_MARCO://UNICO ACCESO

				marco = recibir_entero(socket_memoria);//finaliza 1er acceso

				log_info(logger,"Me llego el marco que es %d",marco);

				if(list_size(tlb) < cantidadEntradasTlb){
					agregarEntradaATLB(marco,numeroDePagina,pcb->idProceso,numeroDeSegmento);
					sleep(1);// Para que espere haya 1 seg de diferencia( a veces pasa que se agregan en el mismo seg y jode los algoritmos)
				}else{
					algoritmosDeReemplazoTLB(numeroDePagina,marco,pcb->idProceso,numeroDeSegmento);
				}
				seAccedeAMemoria = 0;//salga del while
				break;
			case MEMORIA_A_CPU_PAGE_FAULT:
				recibir_mensaje(socket_memoria);
				seAccedeAMemoria = 0;//salga del while
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				seAccedeAMemoria = 0;//salga del while
				break;
		}
	}

	return marco;
}

void leerTamanioDePaginaYCantidadDeEntradas(t_list* listaQueContieneTamanioDePagYEntradas){
	log_info(logger, "Me llegaron los siguientes valores:");

	tamanioDePagina = (int) list_get(listaQueContieneTamanioDePagYEntradas,0);
	entradasPorTabla = (int) list_get(listaQueContieneTamanioDePagYEntradas,1);

	log_info(logger,"tamanio de pagina: %d",tamanioDePagina);
	log_info(logger,"entradas por tabla: %d",entradasPorTabla);
}

void bloqueoPorPageFault(t_pcb* pcb){
	pthread_mutex_lock(&mutexEjecutar);
	ejecutando = false;
	pthread_mutex_unlock(&mutexEjecutar);
	//pcb->programCounter -= 1;
	log_info(loggerObligatorio, "Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>",
			pcb->idProceso, numeroDeSegmento, numeroDePagina);

	enviar_pcb(pcb, CPU_A_KERNEL_PCB_PAGE_FAULT,clienteKernel);
	t_paquete *paquetePageFault = crear_paquete(CPU_A_KERNEL_PAGINA_PF);

	agregar_a_paquete_unInt(paquetePageFault, &numeroDePagina, sizeof(int));
	agregar_a_paquete_unInt(paquetePageFault, &numeroDeSegmento,sizeof(int));


	log_info(logger, "Envio el pcb a kernel por PF");

	enviar_paquete(paquetePageFault, clienteKernel);
	eliminar_paquete(paquetePageFault);
}





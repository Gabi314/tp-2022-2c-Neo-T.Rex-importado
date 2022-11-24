#include "funcionesCpu.h"
#include "tlb.h"

//------------------DECLARO VARIABLES
t_log* logger;
t_config* config;

int cantidadEntradasTlb;
char* algoritmoReemplazoTlb;
int retardoDeInstruccion;

t_pcb* unPcb;
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
static pthread_mutex_t mutexInterrupcion;
pthread_t hiloInterrupciones;


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
}


instruccion* buscarInstruccionAEjecutar(t_pcb* unPCB){//FETCH CREO QUE ES IGUAL

	instruccion* unaInstruccion = list_get(unPCB->instrucciones,unPCB->programCounter);
	unPCB->programCounter++;

	return unaInstruccion;
}
/*Decode
Esta etapa consiste en interpretar qué instrucción es la que se va a ejecutar y si la misma
requiere de un acceso a memoria o no.
REQUIEREN USLEEP-> SET, ADD, MOVIN Y MOVOUT EN CASO DE NO ESTAR EN TLB

En el caso de las instrucciones que no accedan a memoria, las mismas deberán esperar un tiempo
definido por archivo de configuración (RETARDO_INSTRUCCION), a modo de simular el tiempo que transcurre en la CPU.
Las instrucciones de I/O y EXIT al representar syscalls, no tendrán retardo de instrucción.
*/

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
				log_info(logger,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
						pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
							segundoParametro);
				registro = registroAUtilizar(primerParametro,pcb->registros); //En set el primer parametro es el registro
				registro = (uint32_t) segundoParametro; //En set el segundo registro es el valor a asignar
				log_info(logger,"----------------FINALIZA SET----------------\n");
				sleep(2);//para probar que interrumpa sino ejecuta todo rapido
				break;
			case ADD:
				log_info(logger,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%s>",
								pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
									imprimirRegistro(segundoParametro));
				registro = registroAUtilizar(primerParametro,pcb->registros);
				segundoRegistro = registroAUtilizar(segundoParametro,pcb->registros);
				registro += segundoRegistro;
				log_info(logger,"----------------FINALIZA ADD----------------\n");
				break;
			case MOV_IN:
				log_info(logger,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
								pcb->idProceso,unaInstruccion->identificador,imprimirRegistro(primerParametro),
									segundoParametro);

				registro = registroAUtilizar(primerParametro,pcb->registros);
				direccionLogica = segundoParametro;

				if(calculoDireccionLogicaExitoso(direccionLogica,pcb->tablaSegmentos)){
					marco = buscarDireccionFisica(pcb);
					uint32_t valorAAlmacenar;// = funcion que devuelve de memoria el valor mandando el marco conseguido previamente
					registro = valorAAlmacenar;
				}else{
					log_info(logger,"Error: Segmentation Fault (segmento nro: %d)",numeroDeSegmento);//Enviar este mensaje a kernel y devolver pcb
				//PROBAAAR!!!!
				}

				log_info(logger,"----------------FINALIZA MOV_IN----------------\n");
				break;
			case MOV_OUT:
				log_info(logger,"----------------EXECUTE MOV_OUT----------------");
				direccionLogica = primerParametro;
				registro = registroAUtilizar(segundoParametro,pcb->registros);
				marco = buscarDireccionFisica(pcb);
				//funcion que envie valor de registro y que lo guarde en memoria
				enviarDireccionFisica(marco,desplazamientoDePagina,0);//con 0 envia la dir fisica para escribir
				//enviarValorAEscribir(primerParametro); Falta hacer esta funcion

				int cod_op = recibir_operacion(socket_memoria);

				if(cod_op == MENSAJE_CPU_MEMORIA){// Recibe que se escribio correctamente el valor en memoria
					recibir_mensaje(socket_memoria);
					log_info(logger,"----------------FINALIZA MOV_OUT----------------\n");
				}

				break;
			case IO:
				agregar_a_paquete_kernel_cpu(pcb, CPU_PCB_A_KERNEL_POR_IO, paquete); //ver que lo reciba kernel
				enviar_entero(primerParametro,clienteKernel,CPU_DISPOSITIVO_A_KERNEL);//Mandar asi por separado o todo junto?

				if(primerParametro != TECLADO || primerParametro != PANTALLA){

					log_info(logger,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%d>",
							pcb->idProceso, unaInstruccion->identificador,
							dispositivoIOSegunParametro(primerParametro),
							segundoParametro);

					enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_UNIDADES_DE_TRABAJO_IO);
				} else {

					log_info(logger,"“PID: <%d> - Ejecutando: <%s> - <%s> - <%s>",
							pcb->idProceso, unaInstruccion->identificador,
							dispositivoIOSegunParametro(primerParametro),
							imprimirRegistro(segundoParametro));

					if(primerParametro == TECLADO){
						enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_INGRESAR_VALOR_POR_TECLADO);
					}else if(primerParametro == PANTALLA){
						enviar_entero(segundoParametro, clienteKernel, CPU_A_KERNEL_MOSTRAR_REGISTRO_POR_PANTALLA);
					}
				}

				log_info(logger,"----------------FINALIZA I/O----------------\n");
				break;
			case EXT:
				log_info(logger,"“PID: <%d> - Ejecutando: <%s>",pcb->idProceso,unaInstruccion->identificador);
				ejecutando = false;
				//Se deberá devolver el PCB actualizado al Kernel para su finalización.
				log_info(logger,"----------------FINALIZA EXIT----------------\n");
				break;
		}
	}else{
		log_info(logger,"Interrupcion!!!");
		ejecutando = false;
		//devolver pcb
		//hayInterrupcion = false ?
	}
}

void checkInterrupt(){


	pthread_mutex_init(&mutexInterrupcion, NULL);
	pthread_create(&hiloInterrupciones, NULL, (void*) escucharInterrupciones,
			NULL);
	//pthread_detach(hiloInterrupciones);


}

int escucharInterrupciones(){
	log_info(logger, "Cpu escuchando interrupciones");
		while (1) {
			int cod_op = recibir_operacion(clienteKernelInterrupt);
			log_info(logger, "Entro");
			if (cod_op == KERNEL_MENSAJE_INTERRUPT) {
				pthread_mutex_lock(&mutexInterrupcion);
				recibir_mensaje(clienteKernelInterrupt);
				hayInterrupcion = true;
				pthread_mutex_unlock(&mutexInterrupcion);
				return EXIT_SUCCESS;
			} else if (cod_op == -1) {
				log_info(logger, "Se desconecto el kernel. Terminando conexion");
				return EXIT_SUCCESS;
			}
		}
		return EXIT_SUCCESS;
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

	usleep(retardoDeInstruccion);
	return marco;
}

int chequearMarcoEnTLB(int nroDePagina, int nroDeSegmento,int pid){
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));

	for(int i=0;i < list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);

		if(unaEntradaTLB->nroDePagina == nroDePagina
				&& unaEntradaTLB->nroDeSegmento == nroDeSegmento
				&& unaEntradaTLB->nroDeProceso == pid){

			unaEntradaTLB->ultimaReferencia = time(NULL);// se referencio a esa pagina
			return unaEntradaTLB->nroDeMarco; // se puede almacenar este valor en una variable y retornarlo fuera del for
		}
	}
	//free(unaEntradaTLB);
	return -1;
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



	/*
	if(! strcmp(unaInstruccion->identificador,"WRITE")){
		log_info(logger,"----------------EXECUTE WRITE----------------");

		int marco = buscarDireccionFisica(unaInstruccion->parametros[0]);// se podria enviar la pagina para saber en memoria cual se modifica
		enviarDireccionFisica(marco,desplazamiento,0);//con 0 envia la dir fisica para escribir
		enviarValorAEscribir(unaInstruccion->parametros[1]);

		int cod_op = recibir_operacion(conexionMemoria);

		if(cod_op == MENSAJE_CPU_MEMORIA){// Recibe que se escribio correctamente el valor en memoria
			recibir_mensaje(conexionMemoria);
			log_info(logger,"----------------FINALIZA WRITE----------------\n");
		}

*/
int accederAMemoria(int marco,int numeroDeSegmento,t_pcb* pcb){

	log_info(logger,"La pagina no se encuentra en tlb, se debera acceder a memoria(tabla de paginas)"); // dice que la 0 no esta en tlb DEBUGGEAR
	enviarNroTablaDePaginas(pcb->tablaSegmentos,numeroDeSegmento,socket_memoria);//1er acceso con esto memoria manda nroTabla2doNivel

	int seAccedeAMemoria = 1;

	while (seAccedeAMemoria == 1) {
		int cod_op = recibir_operacion(socket_memoria);

		switch (cod_op){
			case PRIMER_ACCESO://UNICO ACCESO

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
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				seAccedeAMemoria = 0;//salga del while
				break;
		}
	}

	return marco;
}


//	}else if(! strcmp(unaInstruccion->identificador,"READ")){
//		log_info(logger,"----------------EXECUTE READ----------------");
//
//		int marco = buscarDireccionFisica(unaInstruccion->parametros[0]);
//		enviarDireccionFisica(marco,desplazamiento,1);//con 1 enviar dir fisica para leer
//
//		log_info(logger,"----------------FINALIZA READ----------------\n");
//
//	}else if(! strcmp(unaInstruccion->identificador,"COPY")){
//		log_info(logger,"----------------EXECUTE COPY----------------");
//		int marcoDeOrigen = buscarDireccionFisica(unaInstruccion->parametros[1]);//DEBUGGEAR
//		int desplazamientoOrigen = desplazamiento;
//
//		int marcoDeDestino = buscarDireccionFisica(unaInstruccion->parametros[0]);
//		int desplazamientoDestino = desplazamiento;
//
//		enviarDireccionesFisicasParaCopia(marcoDeDestino,desplazamientoDestino,marcoDeOrigen,desplazamientoOrigen);
//		log_info(logger,"----------------FINALIZA COPY----------------\n");
//
//	}else if(! strcmp(unaInstruccion->identificador,"NO_OP")){
//		sleep(retardoDeNOOP/1000);// miliseg
//
//	}else if(! strcmp(unaInstruccion->identificador,"I/O")){
//
//		log_info(logger,"----------------I/O-----------------------");
//		//enviarTiempoIO(unaInstruccion->parametros[0]/1000);
//		//enviar_mensaje("Se suspende el proceso",conexionMemoria,MENSAJE);//Se envia pcb a kernel solamente
//		enviarPcb(unPcb,I_O);
//		//bloqueado = 1; esto no se
//		// luego kernel le avisa a memoria que se suspende
//		reiniciarTLB();
//
//		//log_info(logger,"----------------FIN DE I/O----------------"); esto debe ir cuando vuelve de la I/O
//
//	}else if(! strcmp(unaInstruccion->identificador,"EXIT")){
//		// enviar pcb actualizado finaliza el proceso
//		enviarPcb(unPcb,EXIT);
//		log_info(logger,"Finalizo el proceso ");
//		hayInstrucciones = 0;
//	}
//}



void leerTamanioDePaginaYCantidadDeEntradas(t_list* listaQueContieneTamanioDePagYEntradas){
	log_info(logger, "Me llegaron los siguientes valores:");

	tamanioDePagina = (int) list_get(listaQueContieneTamanioDePagYEntradas,0);
	entradasPorTabla = (int) list_get(listaQueContieneTamanioDePagYEntradas,1);

	log_info(logger,"tamanio de pagina: %d ",tamanioDePagina);
	log_info(logger,"entradas por tabla: %d \n",entradasPorTabla);
}

//-------------------------FUNCIONES TLB
//Lista de entradas de TLB
t_list* inicializarTLB(){
	tlb = list_create();
	return tlb;
}

//Se utiliza?
void reiniciarTLB(){
	list_clean(tlb);
}


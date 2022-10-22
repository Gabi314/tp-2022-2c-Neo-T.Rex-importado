#include "funcionesCpu.h"

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

t_list* tlb;

uint32_t ax;
uint32_t bx;
uint32_t cx;
uint32_t dx;

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
	puertoMemoria = config_get_int_value(config,"PUERTO_MEMORIA");

	cantidadEntradasTlb = config_get_int_value(config,"ENTRADAS_TLB");
	algoritmoReemplazoTlb = config_get_string_value(config,"REEMPLAZO_TLB");
	retardoDeInstruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");

	puertoDeEscuchaDispatch = config_get_int_value(config,"PUERTO_ESCUCHA_DISPATCH");
	puertoDeEscuchaInterrupt = config_get_int_value(config,"PUERTO_ESCUCHA_INTERRUPT");
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

uint32_t registroAUtilizar(int enteroDeRegistro){
	if(enteroDeRegistro == AX){
		return ax;
	}
	if(enteroDeRegistro == BX){
		return bx;
	}
	if(enteroDeRegistro == CX){
		return cx;
	}
	if(enteroDeRegistro == DX){
		return dx;
	}

	return -1;

}

int chequeoDeDispositivo(char* registro){
	if(!strcmp(registro,"DISCO"))
	return DISCO;
	if(!strcmp(registro,"PANTALLA"))
	return PANTALLA;
	if(!strcmp(registro,"TECLADO"))
	return TECLADO;

	return -1;
}

/*
 Execute
En este paso se deberá ejecutar lo correspondiente a cada instrucción:
SET (Registro, Valor): Asigna al registro el valor pasado como parámetro.
ADD (Registro Destino, Registro Origen): Suma ambos registros y deja el resultado en el Registro Destino.
MOV_IN (Registro, Dirección Lógica): Lee el valor de memoria del segmento de Datos correspondiente a la Dirección Lógica y lo almacena en el Registro.
MOV_OUT (Dirección Lógica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria del segmento de Datos obtenida
a partir de la Dirección Lógica.
I/O (Dispositivo, Registro / Unidades de trabajo): Esta instrucción representa una syscall de I/O bloqueante. Se deberá devolver el
Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea utilizar el
proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).
EXIT: Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización.


Cabe aclarar que todos los valores a leer/escribir en memoria serán numéricos enteros no signados de 4 bytes, considerar el uso de uint32_t.*/


void ejecutar(instruccion* unaInstruccion,t_list* listaTablaSegmentos){ //
	int primerParametro = unaInstruccion->parametros[0];
	int segundoParametro = unaInstruccion->parametros[1];
	int direccionLogica;
	int marco; //Warning porque no se usa en todos los case
	uint32_t registro;
	uint32_t segundoRegistro;
	switch(decode(unaInstruccion)){
	case SET:
		log_info(logger,"----------------EXECUTE SET----------------");
		registro = registroAUtilizar(primerParametro); //En set el primer parametro es el registro
		registro = (uint32_t) segundoParametro; //En set el segundo registro es el valor a asignar
		log_info(logger,"----------------FINALIZA SET----------------\n");
		break;
	case ADD:
		log_info(logger,"----------------EXECUTE ADD----------------");
		registro = registroAUtilizar(primerParametro);
		segundoRegistro = registroAUtilizar(segundoParametro);
		registro += segundoRegistro;
		log_info(logger,"----------------FINALIZA ADD----------------\n");
		break;
	case MOV_IN:
		log_info(logger,"----------------EXECUTE MOV_IN----------------");
		registro = registroAUtilizar(primerParametro);
		direccionLogica = segundoParametro;
		marco = buscarDireccionFisica(direccionLogica,listaTablaSegmentos);
		uint32_t valorAAlmacenar;// = funcion que devuelve de memoria el valor mandando el marco conseguido previamente
		registro = valorAAlmacenar;
		log_info(logger,"----------------FINALIZA MOV_IN----------------\n");
		break;
	case MOV_OUT:
		log_info(logger,"----------------EXECUTE MOV_OUT----------------");
		direccionLogica = primerParametro;
		registro = registroAUtilizar(segundoParametro);
		marco = buscarDireccionFisica(direccionLogica,listaTablaSegmentos);
		//funcion que envie valor de registro y que lo guarde en memoria

		log_info(logger,"----------------FINALIZA MOV_OUT----------------\n");
		break;
	case IO:
		log_info(logger,"----------------EXECUTE I/O----------------");
		//Primer parametro es el dispositivo
		//Segundo parametro es el registro si es teclado o pantalla o unidades de trabajo
		//Se deberá devolver el Contexto de Ejecución actualizado al Kernel junto el dispositivo y
		//la cantidad de unidades de trabajo del dispositivo que desea utilizar el proceso (o el Registro a completar o
		//leer en caso de que el dispositivo sea Pantalla o Teclado).

		log_info(logger,"----------------FINALIZA I/O----------------\n");
		break;
	case EXT:
		log_info(logger,"----------------EXECUTE EXIT----------------");
		//Se deberá devolver el PCB actualizado al Kernel para su finalización.
		log_info(logger,"----------------FINALIZA EXIT----------------\n");
		break;
	}
}

void checkInterrupt(){
//En este momento, se deberá chequear si el Kernel nos envió una interrupción. De ser el caso,
//se devuelve el Contexto de Ejecución actualizado al Kernel. De lo contrario se reinicia el ciclo de instrucción.

}

int buscarDireccionFisica(int direccionLogica,t_list* listaTablaSegmentos){
	calculosDireccionLogica(direccionLogica,listaTablaSegmentos);
	int marco = chequearMarcoEnTLB(numeroDePagina);
	marco = -1;
//-------------------REVISAR CON LEO
	if (marco == -1){
		return marco; /*= accederAMemoria(marco)*/
	}

	usleep(retardoDeInstruccion);
	return marco;
}

int chequearMarcoEnTLB(int numeroDePagina){
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));

	for(int i=0;i < list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);

			if(unaEntradaTLB->nroDePagina == numeroDePagina){
				unaEntradaTLB->ultimaReferencia = time(NULL);// se referencio a esa pagina
				return unaEntradaTLB->nroDeMarco; // se puede almacenar este valor en una variable y retornarlo fuera del for
			}
	}
	//free(unaEntradaTLB);
	return -1;
}

void calculosDireccionLogica(int direccionLogica,t_list* listaTablaSegmentos){
	tamanioMaximoDeSegmento = entradasPorTabla * tamanioDePagina;
	numeroDeSegmento = floor(direccionLogica / tamanioMaximoDeSegmento);
	desplazamientoDeSegmento = direccionLogica % tamanioMaximoDeSegmento;
	numeroDePagina = floor(desplazamientoDeSegmento  / tamanioDePagina);
	desplazamientoDePagina = desplazamientoDeSegmento % tamanioDePagina;

	chequeoDeSF(numeroDeSegmento,desplazamientoDeSegmento,listaTablaSegmentos);
}
void chequeoDeSF(int numeroDeSegmento, int desplazamientoSegmento,t_list * listaDeTablaSegmentos){
	entradaTablaSegmento* unaEntradaDeTabla;
	for(int i; i< list_size(listaDeTablaSegmentos);i++){
		unaEntradaDeTabla = list_get(listaDeTablaSegmentos,i);
		if(numeroDeSegmento == unaEntradaDeTabla->numeroSegmento){
			if(desplazamientoSegmento>unaEntradaDeTabla->tamanioSegmento){
				//hay SF->>>>> devolverse el proceso al Kernel para que este lo finalice con motivo de Error: Segmentation Fault
			}
		}
	}
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

int buscarDireccionFisica(int direccionLogica){
	calculosDireccionLogica(direccionLogica);
	int marco = chequearMarcoEnTLB(numeroDePagina);

	if (marco == -1){
		marco = accederAMemoria(marco);
	}

	return marco;

}

int chequearMarcoEnTLB(int numeroDePagina){
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));

	for(int i=0;i < list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);

			if(unaEntradaTLB->nroDePagina == numeroDePagina){
				unaEntradaTLB->ultimaReferencia = time(NULL);// se referencio a esa pagina
				return unaEntradaTLB->nroDeMarco; // se puede almacenar este valor en una variable y retornarlo fuera del for
			}
	}
	//free(unaEntradaTLB);
	return -1;
}


int accederAMemoria(int marco){

	log_info(logger,"La pagina no se encuentra en tlb, se debera acceder a memoria(tabla de paginas)"); // dice que la 0 no esta en tlb DEBUGGEAR
	enviarEntradaTabla1erNivel();//1er acceso con esto memoria manda nroTabla2doNivel

	int seAccedeAMemoria = 1;

	while (seAccedeAMemoria == 1) {
		int cod_op = recibir_operacion(conexionMemoria);

		t_list* listaQueContieneNroTabla2doNivel = list_create();
		t_list* listaQueContieneMarco = list_create();

		switch (cod_op){
			case PRIMER_ACCESO://PRIMER_ACCESO

				listaQueContieneNroTabla2doNivel = recibir_paquete(conexionMemoria);//finaliza 1er acceso
				int nroTabla2doNivel = (int) list_get(listaQueContieneNroTabla2doNivel,0);
				log_info(logger,"Me llego el numero de tabla de segundoNivel que es % d",nroTabla2doNivel);

				enviarEntradaTabla2doNivel(); //2do acceso a memoria
				break;
			case SEGUNDO_ACCESO://SEGUNDO_ACCESO
				listaQueContieneMarco = recibir_paquete(conexionMemoria);//Finaliza el 2do acceso recibiendo el marco
				marco = (int) list_get(listaQueContieneMarco,0);

				log_info(logger,"Me llego el marco que es %d",marco);

				if(list_size(tlb) < cantidadEntradasTlb){
					agregarEntradaATLB(marco,numeroDePagina);
					sleep(1);// Para que espere haya 1 seg de diferencia( a veces pasa que se agregan en el mismo seg y jode los algoritmos)
				}else{
					algoritmosDeReemplazoTLB(numeroDePagina,marco);
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


	}else if(! strcmp(unaInstruccion->identificador,"READ")){
		log_info(logger,"----------------EXECUTE READ----------------");

		int marco = buscarDireccionFisica(unaInstruccion->parametros[0]);
		enviarDireccionFisica(marco,desplazamiento,1);//con 1 enviar dir fisica para leer

		log_info(logger,"----------------FINALIZA READ----------------\n");

	}else if(! strcmp(unaInstruccion->identificador,"COPY")){
		log_info(logger,"----------------EXECUTE COPY----------------");
		int marcoDeOrigen = buscarDireccionFisica(unaInstruccion->parametros[1]);//DEBUGGEAR
		int desplazamientoOrigen = desplazamiento;

		int marcoDeDestino = buscarDireccionFisica(unaInstruccion->parametros[0]);
		int desplazamientoDestino = desplazamiento;

		enviarDireccionesFisicasParaCopia(marcoDeDestino,desplazamientoDestino,marcoDeOrigen,desplazamientoOrigen);
		log_info(logger,"----------------FINALIZA COPY----------------\n");

	}else if(! strcmp(unaInstruccion->identificador,"NO_OP")){
		sleep(retardoDeNOOP/1000);// miliseg

	}else if(! strcmp(unaInstruccion->identificador,"I/O")){

		log_info(logger,"----------------I/O-----------------------");
		//enviarTiempoIO(unaInstruccion->parametros[0]/1000);
		//enviar_mensaje("Se suspende el proceso",conexionMemoria,MENSAJE);//Se envia pcb a kernel solamente
		enviarPcb(unPcb,I_O);
		//bloqueado = 1; esto no se
		// luego kernel le avisa a memoria que se suspende
		reiniciarTLB();

		//log_info(logger,"----------------FIN DE I/O----------------"); esto debe ir cuando vuelve de la I/O

	}else if(! strcmp(unaInstruccion->identificador,"EXIT")){
		// enviar pcb actualizado finaliza el proceso
		enviarPcb(unPcb,EXIT);
		log_info(logger,"Finalizo el proceso ");
		hayInstrucciones = 0;
	}
}
*/


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


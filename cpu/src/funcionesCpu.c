#include "funcionesCpu.h"

//------------------DECLARO VARIABLES
t_log* logger;
t_config* config;

int cantidadEntradasTlb;
char* algoritmoReemplazoTlb;
int retardoDeInstruccion;
int puertoDeEscuchaDispath;
int puertoDeEscuchaInterrupt;

t_pcb* unPcb;
int tamanioDePagina;
int entradasPorTabla;

int numeroDeSegmento;
int desplazamientoDeSegmento;
int tamanioMaximoDeSegmento;
int numeroDePagina;
int desplazamientoDePagina;


//--------------------FUNCIONES ELEMENTALES------------------------------
int chequeoCantidadArchivos(int argc){
	if(argc < 2) { //ES 1 PORQUE SOLO TIENE CONFIG??
		    log_error(logger,"Falta un parametro");
		    return EXIT_FAILURE;
		}
}

void inicializarConfiguraciones(char* unaConfig){
	config = config_create(unaConfig);// ver bien como recibir los path de config por parametros

	ipMemoria = config_get_string_value(config,"IP_MEMORIA");
	puertoMemoria = config_get_int_value(config,"PUERTO_MEMORIA");

	cantidadEntradasTlb = config_get_int_value(config,"ENTRADAS_TLB");
	algoritmoReemplazoTlb = config_get_string_value(config,"REEMPLAZO_TLB");
	retardoDeInstruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");

	puertoDeEscuchaDispath = config_get_int_value(config,"PUERTO_ESCUCHA_DISPATCH");
	puertoDeEscuchaInterrupt = config_get_int_value(config,"PUERTO_ESCUCHA_INTERRUPT");
}


instruccion* buscarInstruccionAEjecutar(t_pcb* unPCB){//FETCH CREO QUE ES IGUAL

	instruccion* unaInstruccion = list_get(unPCB->instrucciones,unPCB->programCounter);
	unPCB->programCounter++;

	return unaInstruccion;
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

void calculosDireccionLogica(int direccionLogica){
	tamanioMaximoDeSegmento = entradasPorTabla * tamanioDePagina;
	numeroDeSegmento = floor(direccionLogica / tamanioMaximoDeSegmento);
	desplazamientoDeSegmento = direccionLogica % tamanioMaximoDeSegmento;
	numeroDePagina = floor(desplazamientoDeSegmento  / tamanioDePagina);
	desplazamientoDePagina = desplazamientoDeSegmento % tamanioDePagina;
}

int buscarDireccionFisica(int direccionLogica){
	calculosDireccionLogica(direccionLogica);
	int marco = -1;
	//int marco = chequearMarcoEnTLB(numeroDePagina);

	if (marco == -1){
		return marco; /*= accederAMemoria(marco)*/
	}

	usleep(retardoDeInstruccion);
	return marco;
}

void leerTamanioDePaginaYCantidadDeEntradas(t_list* listaQueContieneTamanioDePagYEntradas){
	log_info(logger, "Me llegaron los siguientes valores:");

	tamanioDePagina = (int) list_get(listaQueContieneTamanioDePagYEntradas,0);
	entradasPorTabla = (int) list_get(listaQueContieneTamanioDePagYEntradas,1);

	log_info(logger,"tamanio de pagina: %d ",tamanioDePagina);
	log_info(logger,"entradas por tabla: %d \n",entradasPorTabla);

}

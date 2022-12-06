#include "funcionesConsola.h"

//Parece que hay que declarar las variables en el .c y en el .h
t_log* logger;
t_config* config;
char* ipKernel;
char* puertoKernel;
char** segmentos;
int tiempoPantalla;

int conexion;

//--------------------------------FUNCIONES GENERALES DE CONSOLA---------------------------------
int chequeoCantidadArchivos(int argc){
	if(argc < 2) {
		    log_error(logger,"Falta un parametro");
		    return EXIT_FAILURE;
		}
	return 1;
}

void inicializarConfiguraciones(char* pathConfig){

	config = config_create(pathConfig);

	ipKernel = config_get_string_value(config,"IP_KERNEL");
	puertoKernel = config_get_string_value(config,"PUERTO_KERNEL");
	segmentos = config_get_array_value(config,"SEGMENTOS");
	tiempoPantalla = config_get_int_value(config,"TIEMPO_PANTALLA");

	log_info(logger,"Config creado");
}

void enviarPaqueteTamanioDeSegmentos(){
	t_paquete* paquete = crear_paquete(KERNEL_PAQUETE_TAMANIOS_SEGMENTOS);

	agregarAPaqueteSegmentos(segmentos,paquete);
	// Enviamos el paquete

	enviar_paquete(paquete,conexion);

}

FILE* abrirArchivo(char* pathArchivo){
	FILE* archivo = fopen(pathArchivo, "r");

	if(archivo == NULL){
		log_error(logger,"No se lee el archivo");
	}else{
		log_info(logger,"Se leyo el archivo correctamente");
	}

	return archivo;

}

FILE* recorrerArchivo(char* contenido,FILE* archivo){
	char** lineasDeInstrucciones;
	t_paquete* paquete = crear_paquete(KERNEL_PAQUETE_INSTRUCCIONES);

	while (fscanf(archivo, "%[^\n] ", contenido) != EOF) {

		instruccion* unaInstruccion = malloc(sizeof(instruccion));

		lineasDeInstrucciones = string_split(contenido, " ");

		unaInstruccion->identificador = malloc(strlen(lineasDeInstrucciones[0])+1);
		strcpy(unaInstruccion->identificador, lineasDeInstrucciones[0]);

		dividirInstruccionesAlPaquete(logger,paquete,lineasDeInstrucciones,unaInstruccion);

		int contadorDeLineas = 0;

		while (contadorDeLineas <= string_array_size(lineasDeInstrucciones)){
			free(lineasDeInstrucciones[contadorDeLineas]);
			contadorDeLineas++;
		}

		free(lineasDeInstrucciones);
		free(unaInstruccion->identificador);
		free(unaInstruccion);
	}
	enviarListaInstrucciones(paquete);
	return archivo;
}

void dividirInstruccionesAlPaquete(t_log* logger,t_paquete* paquete,char** lineasDeInstrucciones,instruccion* unaInstruccion){

 if (!strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"ADD") || !strcmp(lineasDeInstrucciones[0],"MOV_IN") || !strcmp(lineasDeInstrucciones[0],"MOV_OUT") || !strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"EXIT")){ //Se usa solamente para que entre con el exit
		//log_info(logger,"instruccion: %s\n",instruccion->identificador);
	 	//unaInstruccion->parametros = malloc(sizeof(parametro));

	 	if (!strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"MOV_IN")){
	 		//LEI QUE I/O PUEDE NO TENER PARAMETROS
			unaInstruccion->parametros[0] = chequeoDeRegistro(lineasDeInstrucciones[1]);
			unaInstruccion->parametros[1]  = atoi(lineasDeInstrucciones[2]);
			//log_info(logger,"Se guardo el registro %d",unaInstruccion->parametros[0]);
		}
	 	if(!strcmp(lineasDeInstrucciones[0],"I/O")){
			unaInstruccion->parametros[0] = parametroIOSegunDispositivo(lineasDeInstrucciones[1]);
			if(unaInstruccion->parametros[0] == 0 || unaInstruccion->parametros[0] == 1){//si es pantalla o teclado guardo el registro
				unaInstruccion->parametros[1] = chequeoDeRegistro(lineasDeInstrucciones[2]);
			} else {//sino guardo el tiempo del dispositivo
				unaInstruccion->parametros[1] = atoi(lineasDeInstrucciones[2]);
			}
	 	}

		if (!strcmp(lineasDeInstrucciones[0],"ADD")){
			unaInstruccion->parametros[0] = chequeoDeRegistro(lineasDeInstrucciones[1]);
			unaInstruccion->parametros[1]  = chequeoDeRegistro(lineasDeInstrucciones[2]);
		}

		if(!strcmp(lineasDeInstrucciones[0],"MOV_OUT")){
			unaInstruccion->parametros[0]  = atoi(lineasDeInstrucciones[1]);
			unaInstruccion->parametros[1]  = chequeoDeRegistro(lineasDeInstrucciones[2]);
		}
		agregar_a_paquete_instrucciones(paquete, unaInstruccion, strlen(unaInstruccion->identificador)+1);//Usar enum para los registros(pienso mandar solo string para el dispositivo de IO)
		//free(unaInstruccion->parametros);
 }

}

void agregar_a_paquete_instrucciones(t_paquete* paquete, instruccion* instruccion, int identificador_length)
{
	void* id = instruccion->identificador;
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + identificador_length + sizeof(int) + sizeof(int[2]));;

	memcpy(paquete->buffer->stream + paquete->buffer->size, &identificador_length, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), id, identificador_length);
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int) + identificador_length, &instruccion->parametros, sizeof(int[2]));

	paquete->buffer->size += identificador_length + sizeof(int) + sizeof(int[2]);
}

int chequeoDeRegistro(char* registro){
	if(!strcmp(registro,"AX"))
	return AX;
	if(!strcmp(registro,"BX"))
	return BX;
	if(!strcmp(registro,"CX"))
	return CX;
	if(!strcmp(registro,"DX"))
	return DX;

	return -1;
}

int parametroIOSegunDispositivo(char* dispositivo){
	if(!strcmp(dispositivo,"PANTALLA"))
	return PANTALLA;
	if(!strcmp(dispositivo,"TECLADO"))
	return TECLADO;

	for(int i=0; i < string_array_size(listaDispositivos); i++){
		if(! strcmp(listaDispositivos[i],dispositivo)){
			return i+2;//sumo 2 porque la posicion 0 y 1 pertence a pantalla y teclado
		}
	}

	return -1;
}

void agregarAPaqueteSegmentos(char** segmentos,t_paquete* paquete){
	int segmentoActual = 0;
	for(int i=0;i<string_array_size(segmentos);i++){
		segmentoActual = atoi(segmentos[i]);
		agregar_a_paquete_unInt(paquete, &segmentoActual, sizeof(int));
	}
}


void enviarListaInstrucciones(t_paquete* paquete){
	// Enviamos el paquete
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
}



void imprimirValorPorPantalla(int cod_op){//Puede estar en un hilo
	int valorAImprimir = 0;

	valorAImprimir = recibir_entero(conexion);

	usleep(tiempoPantalla*1000);

	log_info(logger,"Se imprime el valor %d por pantalla a pedido del kernel",valorAImprimir);

	enviar_mensaje("Se imprimio el valor del registro", conexion, KERNEL_MENSAJE_VALOR_IMPRESO);
}

void solicitudIngresarValorPorTeclado(int cod_op){
	recibir_mensaje(conexion);

	int valorIngresadoPorTeclado = 0;

	log_info(logger,"Ingrese un valor: ");
	scanf("%d",&valorIngresadoPorTeclado);

	log_info(logger,"Se ingreso el valor %d correctamente",valorIngresadoPorTeclado);

	enviar_mensaje("Desbloquear proceso. Ya se ingreso un valor por teclado",conexion,KERNEL_MENSAJE_DESBLOQUEO_TECLADO);

	enviar_entero(valorIngresadoPorTeclado,conexion,KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO);
}

void atenderPeticionesKernel(){
	int noFinalizar = 1;
	while(noFinalizar){
		int cod_op = recibir_operacion(conexion);

		if(cod_op == KERNEL_PAQUETE_VALOR_A_IMPRIMIR){
			imprimirValorPorPantalla(cod_op);
		}else if(cod_op == KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO){
			solicitudIngresarValorPorTeclado(cod_op);
		}else if(cod_op == KERNEL_MENSAJE_FINALIZAR_CONSOLA){
			recibir_mensaje(conexion);
			noFinalizar = 0;
		}
		else{
			noFinalizar = 0;
		}

	}
}






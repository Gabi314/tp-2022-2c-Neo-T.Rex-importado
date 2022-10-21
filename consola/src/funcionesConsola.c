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
	eliminar_paquete(paquete);
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
			unaInstruccion->parametros[0] = chequeoDeDispositivo(lineasDeInstrucciones[1]);
			if(unaInstruccion->parametros[0] == DISCO){
				unaInstruccion->parametros[1] = atoi(lineasDeInstrucciones[2]);
			}else {
				unaInstruccion->parametros[1] = chequeoDeRegistro(lineasDeInstrucciones[2]);
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

int chequeoDeRegistro(char* registro){
	if(!strcmp(registro,"AX"))
	return AX;
	if(!strcmp(registro,"BX"))
	return BX;
	if(!strcmp(registro,"CX"))
	return CX;
	if(!strcmp(registro,"DX"))
	return DX;
}

int chequeoDeDispositivo(char* registro){
	if(!strcmp(registro,"DISCO"))
	return DISCO;
	if(!strcmp(registro,"PANTALLA"))
	return PANTALLA;
	if(!strcmp(registro,"TECLADO"))
	return TECLADO;
}

void agregarAPaqueteSegmentos(char** segmentos,t_paquete* paquete){
	int segmentoActual = 0;
	for(int i=0;i<string_array_size(segmentos);i++){
		segmentoActual = atoi(segmentos[i]);
		agregar_a_paquete_segmentos(paquete, &segmentoActual, sizeof(int));
	}
}

void enviarListaInstrucciones(t_paquete* paquete){
	// Enviamos el paquete
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
}





void imprimirValorPorPantalla(){//Puede estar en un hilo // no hace falta, así está bien
//	t_list* valorAImprimir = list_create();

	int valorAImprimir = recibir_entero(conexion);

	usleep(tiempoPantalla);

	log_info(logger, "termino el usleep()");
//	log_info(logger,"Se imprime el valor %d por pantalla a pedido del kernel",(int) list_get(valorAImprimir,0));
//	list_clean(valorAImprimir);

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

	//Falta enviarlo!!!! // listo!
}

void atenderPeticionesKernel(){
	int noFinalizar = 1;
	while(noFinalizar){
		int cod_op = recibir_operacion(conexion);

		if(cod_op == KERNEL_PAQUETE_VALOR_A_IMPRIMIR){
			imprimirValorPorPantalla();
		}else if(cod_op == KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO){
			solicitudIngresarValorPorTeclado(cod_op);
		}else if(cod_op == KERNEL_MENSAJE_FINALIZAR_CONSOLA){
			recibir_mensaje(conexion);
			noFinalizar = 0;
		}

	}
}





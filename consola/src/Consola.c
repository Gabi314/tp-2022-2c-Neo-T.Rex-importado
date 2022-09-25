#include "funcionesConsola.h"

t_list* listaTamanioSegmentos;

int main(int argc, char *argv[]) {

	logger = log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}

	inicializarConfiguraciones(argv[1]);

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ipKernel, puertoKernel);

	enviarPaqueteTamanioDeSegmentos();

	FILE* archivo = abrirArchivo(argv[2]);

	struct stat sb;
	stat(argv[2], &sb);
	char* contenido = malloc(sb.st_size);

	archivo = recorrerArchivo(contenido,archivo);

	fclose(archivo);
	if (contenido != NULL) //valida si contenido es NULL
	free(contenido);

}

void dividirInstruccionesAlPaquete(t_log* logger,t_paquete* paquete,char** lineasDeInstrucciones,instruccion* unaInstruccion){

 if (!strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"ADD") || !strcmp(lineasDeInstrucciones[0],"MOV_IN") || !strcmp(lineasDeInstrucciones[0],"MOV_OUT") || !strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"EXIT")){ //Se usa solamente para que entre con el exit
		//log_info(logger,"instruccion: %s\n",instruccion->identificador);
	 	unaInstruccion->parametros = malloc(sizeof(parametro));

	 	if (!strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"MOV_IN")){
	 		//LEI QUE I/O PUEDE NO TENER PARAMETROS
			unaInstruccion->parametros->registro = lineasDeInstrucciones[1];
			log_info(logger,"Se guardo el registro %s",unaInstruccion->parametros->registro);
			unaInstruccion->parametros->valor  = atoi(lineasDeInstrucciones[2]);
		}

		if (!strcmp(lineasDeInstrucciones[0],"ADD")){
			unaInstruccion->parametros->registro  = lineasDeInstrucciones[1];
			unaInstruccion->parametros->otroRegistro  = lineasDeInstrucciones[2];
		}

		if(!strcmp(lineasDeInstrucciones[0],"MOV_OUT")){
			unaInstruccion->parametros->valor  = atoi(lineasDeInstrucciones[1]);
			unaInstruccion->parametros->registro  = lineasDeInstrucciones[2];
		}

		agregar_a_paquete_instrucciones(paquete, unaInstruccion, strlen(unaInstruccion->identificador)+1);//Usar enum para los registros(pienso mandar solo string para el dispositivo de IO)
		free(unaInstruccion->parametros);
 }

}


void agregarAPaqueteSegmentos(char** segmentos,t_paquete* paquete){
	int segmentoActual = 0;

	for(int i=0;i<string_array_size(segmentos);i++){
		segmentoActual = atoi(segmentos[i]);

		agregar_a_paquete_segmentos(paquete, &segmentoActual, sizeof(int));
	}

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

	return archivo;
}


void inicializarConfiguraciones(char* pathConfig){

	config = config_create(pathConfig);

	ipKernel = config_get_string_value(config,"IP_KERNEL");
	puertoKernel = config_get_string_value(config,"PUERTO_KERNEL");
	segmentos = config_get_array_value(config,"SEGMENTOS");
	log_info(logger,"Config creado");
}

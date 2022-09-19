#include "funcionesConsola.h"


int main(int argc, char *argv[]) {
	logger = log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);

	if(argc < 2) {
	    log_error(logger,"Falta un parametro");
	    return EXIT_FAILURE;
	}

	inicializarConfiguraciones();

	FILE* archivo = fopen(argv[2], "r");

	if(archivo == NULL){
		log_error(logger,"No se lee el archivo");
	}else{
		log_info(logger,"Se leyo el archivo correctamente");
	}

	struct stat sb;
	stat(argv[2], &sb);
	char * contenido = malloc(sb.st_size);;

	char** lineasDeInstrucciones;

	t_paquete* paquete = crear_paquete(INSTRUCCIONES);

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ipKernel, puertoKernel);

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

}

void dividirInstruccionesAlPaquete(t_log* logger,t_paquete* paquete,char** lineasDeInstrucciones,instruccion* unaInstruccion){

 if (!strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"ADD") || !strcmp(lineasDeInstrucciones[0],"MOV_IN") || !strcmp(lineasDeInstrucciones[0],"MOV_OUT") || !strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"EXIT")){ //Se usa solamente para que entre con el exit
		//log_info(logger,"instruccion: %s\n",instruccion->identificador);

		if (!strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"SET") || !strcmp(lineasDeInstrucciones[0],"MOV_IN")){
			unaInstruccion->parametros->registro = lineasDeInstrucciones[1];
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

		agregar_a_paquete(paquete, unaInstruccion, strlen(unaInstruccion->identificador)+1);
	}

}

void inicializarConfiguraciones(){
	config = config_create("consola.config");

	ipKernel = config_get_string_value(config,"IP_KERNEL");
	puertoKernel = config_get_string_value(config,"PUERTO_KERNEL");
	segmentos = config_get_array_value(config,"SEGMENTOS");
}

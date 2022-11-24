#include "funcionesConsola.h"

char** listaDispositivos;

int main(int argc, char *argv[]) {
	//pthread_t hiloConexionKernel;

	logger = log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);
	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	//Parseo config
	inicializarConfiguraciones(argv[1]);

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ipKernel, puertoKernel);
	//pthread_create(&hiloConexionKernel, NULL,(void*) enviarPaqueteTamanioDeSegmentos, ipKernel);

	//Envio los tamanios de segmentos recibidos por config
	enviarPaqueteTamanioDeSegmentos();

	listaDispositivos = recibirListaDispositivos(conexion);

	FILE* archivo = abrirArchivo(argv[2]);

	struct stat sb;
	stat(argv[2], &sb);
	char* contenido = malloc(sb.st_size);


	archivo = recorrerArchivo(contenido,archivo);

	atenderPeticionesKernel();

	fclose(archivo);
	if (contenido != NULL) //valida si contenido es NULL
	free(contenido);


}







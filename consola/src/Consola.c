#include "funcionesConsola.h"
#include <pthread.h>

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

	//Ver como meter esto en una funcion?
	FILE* archivo = abrirArchivo(argv[2]);

	struct stat sb;
	stat(argv[2], &sb);
	char* contenido = malloc(sb.st_size);

	//solicitudIngresarValorPorTeclado();

	archivo = recorrerArchivo(contenido,archivo);

	fclose(archivo);
	if (contenido != NULL) //valida si contenido es NULL
	free(contenido);

	atenderPeticionesKernel();

}







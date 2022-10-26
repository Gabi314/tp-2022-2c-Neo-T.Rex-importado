#include "funcionesConsola.h"

int main(int argc, char *argv[]) {

	logger = log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);
	//Chequeo cantidad de archivos recibidos en el main
	chequeoCantidadArchivos(argc);

	//Parseo config
	inicializarConfiguraciones(argv[1]);

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ipKernel, puertoKernel);

	//Envio los tamanios de segmentos recibidos por config
	enviarPaqueteTamanioDeSegmentos();

	//Ver como meter esto en una funcion?
	FILE* archivo = abrirArchivo(argv[2]);

	struct stat sb;
	stat(argv[2], &sb);
	char* contenido = malloc(sb.st_size);

	//solicitudIngresarValorPorTeclado();

	archivo = recorrerArchivo(contenido,archivo);
	int codigo = recibir_operacion(conexion);

	if(codigo != KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS){
		log_info(logger,"codigo de operacion incorrecto: %d", codigo);
	}
	recibir_mensaje(conexion);
	atenderPeticionesKernel();

	fclose(archivo);
	if (contenido != NULL) //valida si contenido es NULL
	free(contenido);
	//Hasta aca

}







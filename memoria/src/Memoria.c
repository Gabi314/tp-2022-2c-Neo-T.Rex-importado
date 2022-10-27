#include "funcionesMemoria.h"

/*ANOTACIONES DE MEMORIA
 * Un unico archivo de swap para TODO EL SISTEMA
 *
 * Paginacion bajo demanda-> cada segmento posee tabla de paginas propia
 * Estructura es: numero de marco, bit de presencia, bit de uso, bit de modificado
 * Y posicion en SWAP
 * Cantidad fija de marcos por proceso Y REEMPLAZO LOCAL
 *
 * Clock - Clock M
 *
 * Estructuras: void memoria, tablas de paginas y unico archivo de SWAP
 *
 * Cuando se accede a tabla de paginas el modulo debe responder el marco y si no se
 * encuentra debe retornar PF
 * PF: modulo debera obtener pagina asociada de SWAP y escribirla en la memoria principal
 *
 * Ante pedido de lectura, devolver el valor de posicion
 * Ante pedido de escritura, escribir en posicion y devolver "OK"
 * CADA ACCESO A ESPACIO DE USUARIO, USAR RETARDO, NO USADO EN OP DE KERNEL
 *
 * INICIO DE MEMORIA-> SE GENERA SWAP, si existe se elimina y se crea
 * TODOS LOS ACCESOS A SWAP DEBEN POSEER RETARDO (todas las op de lectura y escritura)
 *
 *
 *
 * */

int main(int argc, char *argv[]) {
	//Obligatorios
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	listaDeMarcos = list_create();
	listaDeEntradasEnMemoria = list_create();
	listaTablaDePaginas = list_create();
	listaDePaginasEnMemoria = list_create();
	chequeoCantidadArchivos(argc);
	crearConfiguraciones(argv[1]);
	inicializarMemoria();
	inicializarMarcos();
	//Obligatorios


	//log_info(logger,"Cantidad de marcos %d",list_size(listaDeMarcos));
	log_info(logger,"Fin de memoria");
}





















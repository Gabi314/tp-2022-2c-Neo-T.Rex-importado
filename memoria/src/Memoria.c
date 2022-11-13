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
 * Clog_createADA ACCESO A ESPACIO DE USUARIO, USAR RETARDO, NO USADO EN OP DE KERNEL
 *
 * INICIO DE MEMORIA-> SE GENERA SWAP, si existe se elimina y se crea
 * TODOS LOS ACCESOS A SWAP DEBEN POSEER RETARDO (todas las op de lectura y escritura)
 *
 *
 *
 * */
int clienteCpu;

int main(int argc, char *argv[]) {
	//Obligatorios
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	listaDeMarcos = list_create();
	listaDeEntradasEnMemoria = list_create();
	listaTablaDePaginas = list_create();
	//listaDePaginasEnMemoria = list_create();
	chequeoCantidadArchivos(argc);
	crearConfiguraciones(argv[1]);
	inicializarMemoria();
	inicializarMarcos();


	conexionConCpu();

	crearSwap();

	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	unaEntrada->modificado = 0;
	unaEntrada->numeroDeEntrada = 0;
	unaEntrada->posicionEnSwap = 0;
	unaEntrada->presencia = 0;
	unaEntrada->uso = 0;
	cargarPagina(unaEntrada);
	log_info(logger,"numero de marco asignado es: %d", unaEntrada->numeroMarco);

	escribirElPedido(1234,unaEntrada->numeroMarco,8);
	uint32_t* datoLeido = leerElPedido(unaEntrada->numeroMarco,8);
	log_info(logger,"numero leido primero: %d", datoLeido);

	escribirEnSwap(unaEntrada->numeroMarco);

	leerDeSwap(unaEntrada->numeroDeEntrada,unaEntrada->numeroMarco);

	datoLeido = leerElPedido(unaEntrada->numeroMarco,8);
	log_info(logger,"numero leido tercero: %d", datoLeido);

	log_info(logger,"Fin de memoria");
	log_info(logger,"Boca");
}





















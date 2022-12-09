#include "funcionesMemoria.h"

//--------------------------DECLARO VARIABLES
t_log* logger;
t_log* loggerAux;

//Variables globales de config
int tamanioDeMemoria;
int tamanioDePagina;
int entradasPorTabla;
int retardoMemoria;
char* algoritmoDeReemplazo;
int marcosPorProceso;
int retardoSwap;
char* pathSwap;
int tamanioSwap;
char* ipMemoria;
char* puertoMemoria;

//Variables a utilizar
void* memoria; // espacio de usuario de la memoria

int contadorDeEntradasPorProceso;

//CREO QUE NO SE UTILIZA t_list* listaDePaginasEnMemoria;

int posicionDelPuntero;
int contadorDeMarcosPorProceso = 0;
int cantidadDeSegmentos;

int contNroTablaDePaginas = 0;

int posicionActualDeSwap = 0;

//------------------------- DEFINICION DE FUNCIONES
int chequeoCantidadArchivos(int argc) {
	if(argc < 2) {
		log_error(loggerAux, "Falta un parametro");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void crearConfiguraciones(char* unaConfig) {
	t_config* config = config_create(unaConfig);

	if(config == NULL) {
		printf("Error leyendo archivo de configuración. \n");
	}

	tamanioDeMemoria = config_get_int_value(config, "TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config, "TAM_PAGINA");
	// Agregar "IP_MEMORIA" al archivo de config
	puertoMemoria = config_get_string_value(config, "PUERTO_ESCUCHA");

	entradasPorTabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config, "RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config, "MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config, "RETARDO_SWAP");
	pathSwap = config_get_string_value(config, "PATH_SWAP");
	tamanioSwap = config_get_int_value(config, "TAMANIO_SWAP");
}

void inicializarMemoria() {
	memoria = malloc(tamanioDeMemoria); // inicializo el espacio de usuario en memoria
}

//Mandar el cont antes de inicializar estructuras a kernel
void inicializarEstructuras(int pid) {
	for(int i = 0; i < cantidadDeSegmentos; i++) {
		tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
		unaTablaDePaginas->pid = pid;
		unaTablaDePaginas->entradas = list_create();
		unaTablaDePaginas->numeroDeSegmento = i;
		cargarEntradasATabla(unaTablaDePaginas, i);

		//DEJO ESTA LISTA PARA "INICIALIZAR PROCESO", PORQUE DESPUES HAY QUE VER DONDE SE AGREGO ESTA TABLA DE PAGINAS Y MANDARLA A CPU
		list_add(listaTablaDePaginas, unaTablaDePaginas);
		contNroTablaDePaginas++;
		log_info(logger, "PID: <%d> - Segmento: <%d> - TAMAÑO: <%d> paginas", pid, i, entradasPorTabla);
	}
	contNroTablaDePaginas++;
}

void cargarEntradasATabla(tablaDePaginas* unaTablaDePaginas, int numeroDeSegmento) {
	for(int j=0; j < entradasPorTabla; j++) { //crear x cantidad de marcos y agregar de a 1 a la lista
		entradaTablaPaginas* unaEntradaDeTabla = malloc(sizeof(entradaTablaPaginas));

		unaEntradaDeTabla->numeroDeEntrada = j; //Antes era igual a contadorDeEntradasPorProceso, pero como ahora es 1 sola tabla, solo tiene la cantidad = entradasPorTabla
		unaEntradaDeTabla->numeroMarco = -1;
		unaEntradaDeTabla->presencia = 0;
		unaEntradaDeTabla->uso = 0;
		unaEntradaDeTabla->modificado = 0;
		unaEntradaDeTabla->posicionEnSwap = -1;
		unaEntradaDeTabla->numeroDeSegmento = numeroDeSegmento;

		//contadorDeEntradasPorProceso++;

		list_add(unaTablaDePaginas->entradas, unaEntradaDeTabla);
	}
	//contadorDeEntradasPorProceso = 0;
}

void inicializarMarcos() {
	for(int j=0; j < (tamanioDeMemoria/tamanioDePagina); j++) {
		marco* unMarco = malloc(sizeof(marco));
		unMarco->numeroDeMarco = j;
		unMarco->marcoLibre = 0;
		list_add(listaDeMarcos,unMarco);
	}
}

void sacarMarcoAPagina(entradaTablaPaginas* unaEntrada) {
	unaEntrada->presencia = 0;
	unaEntrada->uso = 0;
	//unaEntrada->numeroMarco = -1;
}

void reemplazarTodosLosUsoACero(t_list* listaDeEntradasEnMemoria) {
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		unaEntrada = list_get(listaDeEntradasEnMemoria, i);
		unaEntrada->uso = 0;
	}
}

int punteroAUtilizar(int pid){
	entradaListaPunteros* unaEntrada;
	for(int i=0;i<list_size(listaDePunterosYPids);i++){
		unaEntrada = list_get(listaDePunterosYPids,i);
		if(unaEntrada->pid == pid){
			return unaEntrada->posicionDelPuntero;
		}
	}
}

//FUNCIONES DE LOS 2 ALGORITMOS --> TENER EN CUENTA QUE PASO LA LISTA DE ENTRADAS
int algoritmoClock(t_list* listaDeEntradasEnMemoria, entradaTablaPaginas* entradaACargar,int pid) {
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	posicionDelPuntero = punteroAUtilizar(pid);

	// 0 < listaDeEntradasEnMemoria < CANTIDAD_MARCOS_POR_PROCESO
	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(loggerAux, "PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);

			log_info(logger,"REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
					pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
					entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(loggerAux, "No hay ninguna pagina con bit de uso en 0");
	log_info(loggerAux, "Por algoritmo reemplazo todos los bit de uso a 0 y busco de nuevo");

	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
	reemplazarTodosLosUsoACero(listaDeEntradasEnMemoria);

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente

			log_info(loggerAux,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			log_info(logger,"REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
						pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
						entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}
}

int posicionDePunteroDelAlgoritmo(int i) {
	posicionDelPuntero += i;

	if(posicionDelPuntero<marcosPorProceso) {
		return posicionDelPuntero;
	} else {
		posicionDelPuntero = 0;
		return posicionDelPuntero;
	}
}

int algoritmoClockM (t_list* listaDeEntradasEnMemoria, entradaTablaPaginas* entradaACargar,int pid){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente

			log_info(loggerAux, "PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
						pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
						entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(loggerAux, "No hay ninguna pagina con bit de uso y modificado en 0");
	log_info(loggerAux, "Busco con bit de uso en 0 y modificado en 1");

	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(loggerAux, "PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);

			log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
						pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
						entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(loggerAux, "No hay ninguna pagina con bit de uso en 0 y modificado en 1");
	log_info(loggerAux, "Reemplazo todos los bit de uso en 0");

	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
	reemplazarTodosLosUsoACero(listaDeEntradasEnMemoria);

	log_info(loggerAux, "Busco con bit de uso en 0 y modificado en 0 ");

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(loggerAux, "PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);

			log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
						pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
						entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(loggerAux, "No hay ninguna pagina con bit de uso en 0 y modificado en 0");
	log_info(loggerAux, "Busco con bit de uso en 0 y modificado en 1");

	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);

	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		if(i == 0) {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		} else {
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(loggerAux, "La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria, posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1) {
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(loggerAux, "PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);

			log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
						pidActual, numeroDeMarcoAReemplazar, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada,
						entradaACargar->numeroDeSegmento, entradaACargar->numeroDeEntrada);

			return numeroDeMarcoAReemplazar;
		}
	}
}

marco* siguienteMarcoLibre() {
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0; i < list_size(listaDeMarcos); i++) {

		unMarco = list_get(listaDeMarcos, i);

		if(unMarco->marcoLibre == 0) {
			return unMarco;
		}
	}
}

void modificarPaginaACargar(entradaTablaPaginas* unaEntrada, int nroDeMarcoAASignar) {
	unaEntrada->numeroMarco = nroDeMarcoAASignar;
	unaEntrada->presencia = 1;
	unaEntrada->uso = 1;
	//unaPagina->modificado = 0; ANALIZAR CASO DE MODIFICADO EN 1
	marco* marcoAsignado = buscarMarco(nroDeMarcoAASignar);
	marcoAsignado->marcoLibre = 1;
}

marco* buscarMarco(int nroDeMarco) {
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0; i < list_size(listaDeMarcos); i++) {
		unMarco = list_get(listaDeMarcos, i);
		if(unMarco->numeroDeMarco == nroDeMarco) {
			return unMarco;
		}
	}
	free(unMarco);
}

int indiceDeEntradaAReemplazar(int numeroDeMarco) {
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		unaEntrada = list_get(listaDeEntradasEnMemoria, i);
		if(unaEntrada->numeroMarco == numeroDeMarco) {
			return i;
		}
	}
}

void liberarEspacioEnMemoria(tablaDePaginas* unaTablaDePaginas) {
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	marco* marcoAsignado = malloc(sizeof(marco));
	t_list* listaDeEntradasAux = list_create();
	listaDeEntradasAux = unaTablaDePaginas->entradas;

	for(int j = 0; j < entradasPorTabla; j++) {
		unaEntrada = list_get(listaDeEntradasAux, j);
		if(unaEntrada->presencia == 1 && unaEntrada->modificado == 0) {
			sacarMarcoAPagina(unaEntrada);
			marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
			marcoAsignado->marcoLibre = 0;
		}
		if(unaEntrada->presencia == 1 && unaEntrada->modificado == 1) {
			escribirEnSwap(unaEntrada);

			sacarMarcoAPagina(unaEntrada);
			marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
			marcoAsignado->marcoLibre = 0;
		}
	}
	list_clean(listaDeEntradasEnMemoria);
	//contadorDeMarcosPorProceso = 0;
}

void finalizacionDeProceso(int pid) {
	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	for(int i = 0; i < list_size(listaTablaDePaginas); i++) {
	unaTablaDePaginas = list_get(listaTablaDePaginas, i);
		if(unaTablaDePaginas->pid == pid) {
			liberarEspacioEnMemoria(unaTablaDePaginas);
		}
	}
}

void chequeoCantidadMarcosProceso(int pid){
	tablaDePaginas* unaTablaDePaginas;
	entradaTablaPaginas* unaEntrada;
	contadorDeMarcosPorProceso = 0;
	for(int i = 0;i<list_size(listaTablaDePaginas);i++){

		unaTablaDePaginas = list_get(listaTablaDePaginas,i);

		if(unaTablaDePaginas->pid == pid){

			for(int j=0;j<list_size(unaTablaDePaginas->entradas);j++){

				unaEntrada = list_get(unaTablaDePaginas->entradas,j);
				if(unaEntrada->presencia == 1){
					contadorDeMarcosPorProceso++;
				}
			}
		}
	}
} //REVISAR listaDeEntradasEnMemoria

void cargarPagina(entradaTablaPaginas* unaEntrada,int pid) {
	if(unaEntrada->presencia == 0) {
		marco* marcoAAsignar;
		//int numeroMarcoPrevio = unaEntrada->numeroMarco;

		chequeoCantidadMarcosProceso(pid);
		//Caso en el que se puede asignar un marco a un proceso de manera libre
		if(contadorDeMarcosPorProceso < marcosPorProceso && (list_size(listaDeEntradasEnMemoria) <= (tamanioDeMemoria/tamanioDePagina))) {
			marcoAAsignar = siguienteMarcoLibre();
			modificarPaginaACargar(unaEntrada, marcoAAsignar->numeroDeMarco);
			list_add(listaDeEntradasEnMemoria, unaEntrada);
			//contadorDeMarcosPorProceso++;
			if(unaEntrada->modificado == 1) {
				leerDeSwap(unaEntrada, unaEntrada->numeroMarco);
				unaEntrada->modificado = 0;
			}
			// Temporal
			chequeoCantidadMarcosProceso(pid);
			log_info(loggerAux, "La cantidad de marcos asignados a este proceso es: %d", contadorDeMarcosPorProceso);
			// Hasta aca
			//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1
		}else if(strcmp(algoritmoDeReemplazo, "CLOCK") == 0) {
				int marcoAAsignar = algoritmoClock(listaDeEntradasEnMemoria, unaEntrada);
				//modificarPaginaACargar(unaEntrada, marcoAAsignar);
				//int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
				//list_replace(listaDeEntradasEnMemoria, posicionAReemplazar, unaEntrada);

				if(unaEntrada->modificado == 1) {
					leerDeSwap(unaEntrada, unaEntrada->numeroMarco);
					unaEntrada->modificado = 0;
				}
		} else if(strcmp(algoritmoDeReemplazo, "CLOCK-M") == 0) {
			int marcoAAsignar = algoritmoClockM(listaDeEntradasEnMemoria, unaEntrada);
			//modificarPaginaACargar(unaEntrada, marcoAAsignar);
			//int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			//list_replace(listaDeEntradasEnMemoria, posicionAReemplazar, unaEntrada);

			if(unaEntrada->modificado == 1) {
				leerDeSwap(unaEntrada,unaEntrada->numeroMarco);
				unaEntrada->modificado = 0;
			}
		}
	}
}


void escribirElPedido(uint32_t datoAEscribir, int marco, int desplazamiento) {
	usleep(retardoMemoria*1000);
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy(&memoria+posicion, &datoAEscribir, sizeof(uint32_t));

	log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d>", pidActual, posicion);

	entradaTablaPaginas* entradaAEscribir = entradaCargadaConMarcoAsignado(marco);
	entradaAEscribir->modificado = 1;
	//enviar_mensaje("Se escribio el valor correctamente",clienteCpu);
}

entradaTablaPaginas* entradaCargadaConMarcoAsignado(int nroDeMarco) {
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	for(int i = 0; i < list_size(listaDeEntradasEnMemoria); i++) {
		unaEntrada = list_get(listaDeEntradasEnMemoria, i);

		if(unaEntrada->numeroMarco == nroDeMarco) {
			return unaEntrada;
		}
	}
}

uint32_t leerElPedido(int marco, int desplazamiento) {
	usleep(retardoMemoria*1000);
	uint32_t datoALeer;
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy(&datoALeer, &memoria+posicion, sizeof(uint32_t));
	log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección física: <%d>", pidActual, posicion);
	if(datoALeer != 0) {
		return datoALeer;
	}
}

uint32_t leerEnSwap(int marco, int desplazamiento) {
	usleep(retardoMemoria*1000);
	uint32_t datoALeer;
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy(&datoALeer, &memoria+posicion, sizeof(uint32_t));
	if(datoALeer != 0) {
		return datoALeer;
	}
}

void crearSwap() {
	//char* nombrePathCompleto = nombreArchivoProceso(pid);
	remove(pathSwap);
	FILE* archivoSwap = fopen(pathSwap, "w+");
	truncate(pathSwap, tamanioSwap);
	/*
	for(int i=0; i<(entradasPorTabla*entradasPorTabla);i++){
		for(int i=0; i<(entradasPorTabla*entradasPorTabla);i++){
			fwrite("0000",sizeof(int),1,archivoSwap);
		}
		fputs ("\n", archivoSwap);
	}
	*/
	fclose(archivoSwap);
}

void escribirEnSwap(entradaTablaPaginas* unaEntrada) {
	int numeroDeMarco = unaEntrada->numeroMarco;
	usleep(retardoSwap*1000);
	FILE* archivoSwap = fopen(pathSwap, "r+");

	if(unaEntrada->posicionEnSwap == -1) {
		unaEntrada->posicionEnSwap = posicionActualDeSwap;
	}

	for(int i=0; i < (tamanioDePagina/sizeof(uint32_t)); i++) {
		fseek(archivoSwap, unaEntrada->posicionEnSwap, SEEK_SET);
		fseek(archivoSwap, i*4, SEEK_CUR);

		uint32_t datoAEscribir = leerEnSwap(numeroDeMarco, i*sizeof(uint32_t));
		char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
		fputs(datoAEscribirEnChar,archivoSwap);
	}
	posicionActualDeSwap += tamanioDePagina;
	fclose(archivoSwap);
	log_info(logger, "SWAP OUT - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d>", pidActual,
			numeroDeMarco, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada);
}

void leerDeSwap(entradaTablaPaginas* unaEntrada, int numeroDeMarcoNuevo) {
	usleep(retardoSwap*1000);

	char* parteDePagina = string_new();
	FILE* archivoSwap = fopen(pathSwap, "r");

//	int posicionDeLaPaginaALeer = posicionDePagEnSwap;

	for(int i = 0; i < (tamanioDePagina/sizeof(uint32_t)); i++) {
		fseek(archivoSwap, unaEntrada->posicionEnSwap, SEEK_SET);
		fseek(archivoSwap, i*4, SEEK_CUR);

		fgets(parteDePagina, sizeof(uint32_t)+1, archivoSwap);

		uint32_t parteDePaginaEnInt = atoi(parteDePagina);

		memcpy(&memoria+(tamanioDePagina*numeroDeMarcoNuevo)+i*sizeof(uint32_t), &parteDePaginaEnInt, sizeof(uint32_t));
	}
	fclose(archivoSwap);
	log_info(logger, "SWAP IN - PID: <%d> - Marco: <%d> - Page In: <%d>|<%d", pidActual,
			numeroDeMarcoNuevo, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada);
}

/**************************************** ALGORITMOS DE SUSTITUCION **********************************************/

uint32_t sustitucion_paginas(uint32_t numero_tabla_primer_nivel, uint32_t numero_pagina, size_t pid) {
	t_lista_circular* frames_proceso = obtener_lista_circular_del_proceso(pid);

	if (strcmp("CLOCK", algoritmoDeReemplazo)==0) {
		return algoritmo_clock(frames_proceso, numero_tabla_primer_nivel, numero_pagina);
	}
	else if (strcmp("CLOCK-M", algoritmoDeReemplazo)==0) {
		return algoritmo_clock_modificado(frames_proceso, numero_tabla_primer_nivel, numero_pagina);
	}
	log_info(loggerAux, "Algoritmo de reemplazo invalido");
	return -1;
}

/*
 * Cambios necesarios:
 *
 * En cargarPagina agregar el parametro frames_proceso (busqueda de la lista de entradas cargadas por proceso)
 *
 */
// uint32_t algoritmo_clock(t_lista_circular* frames_proceso, uint32_t numero_tabla_primer_nivel, uint32_t numero_pagina) {
uint32_t algoritmo_clock(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {
	// Variables auxiliares
	t_frame_lista_circular* frame_puntero = malloc(sizeof(t_frame_lista_circular));

	uint hay_victima = 0;

	while(hay_victima == 0) {
		// Al implementar un puntero_algoritmo que se va desplazando dentro de la propia lista circular
		// 		lo reemplazamos por eso
		frame_puntero = frames_proceso->puntero_algoritmo;

		hay_victima = es_victima_clock(frame_puntero->info);

		if (hay_victima) {
			//entrada_tabla_paginas_victima = obtener_entrada_tabla_de_paginas(numero_tabla_de_paginas, frame_puntero->info);
			actualizar_registros(entrada_tabla_paginas, frame_puntero->info);
		} else {
			frame_puntero->info->uso = 0;
		}
		frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
	}
	return frame_puntero->info->numeroMarco;
}

uint32_t algoritmo_clock_modificado(t_lista_circular* frames_proceso, uint32_t numero_tabla_primer_nivel, uint32_t numero_pagina) {
	t_registro_segundo_nivel* registro_segundo_nivel = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, numero_pagina);

	// Variables auxiliares
	t_frame_lista_circular* frame_puntero = malloc(sizeof(t_frame_lista_circular));
	t_registro_segundo_nivel* registro_segundo_nivel_victima = malloc(sizeof(t_registro_segundo_nivel));;
	t_registro_segundo_nivel* registro_segundo_nivel_actualizado = malloc(sizeof(t_registro_segundo_nivel));

	uint hay_victima_um = 0;
	uint hay_victima_u = 0;
	uint busquedas_um = 0;
	uint busquedas_u = 0;

	while (hay_victima_um == 0 && hay_victima_u == 0) {

		busquedas_um = 0;
		busquedas_u = 0;

		// Primeras iteraciones en busqueda de una pagina con U=0 && M=0
		// No se actualiza ningun bit
		while (hay_victima_um == 0 && busquedas_um < marcos_por_proceso) {

			busquedas_um++;

			frame_puntero = frames_proceso->puntero_algoritmo;

			hay_victima_um = es_victima_clock_modificado_um(frame_puntero->info);

			if (hay_victima_um) {
				registro_segundo_nivel_victima = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);

				actualizar_registros(registro_segundo_nivel, registro_segundo_nivel_victima, frame_puntero->info->numero_frame);

				frame_puntero->info->numero_pagina = numero_pagina;
				frame_puntero->info->uso = 1;

				frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
				break;
			}

			frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
		}

		// Si no hay victima de u=0 y m=0
		if (hay_victima_um == 0) {

			// Segundas iteraciones en busqueda de una pagina con U=0 && M=1
			// Se actualiza el bit de uso
			while (hay_victima_u == 0 && busquedas_u < marcos_por_proceso) {

				busquedas_u++;

				frame_puntero = frames_proceso->puntero_algoritmo;

				hay_victima_u = es_victima_clock_modificado_u(frame_puntero->info);

				if (hay_victima_u) {
					registro_segundo_nivel_victima = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);

					actualizar_registros(registro_segundo_nivel, registro_segundo_nivel_victima, frame_puntero->info->numero_frame);

					frame_puntero->info->numero_pagina = numero_pagina;
					frame_puntero->info->uso = 1;

					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
					break;
				} else {
					registro_segundo_nivel_actualizado = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);
					registro_segundo_nivel_actualizado->uso = 0;
					frame_puntero->info->uso = 0;
					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
				}
			}
		}
	}
	return frame_puntero->info->numero_frame;
}

/**************************************** FUNCIONES AUXILIARES ALGORITMOS **********************************************/
uint es_victima_clock(entradaTablaPaginas* entrada) {
	return entrada->presencia == 1 && entrada->uso == 0;
}

uint es_victima_clock_modificado_um(entradaTablaPaginas* entrada) {
	return entrada->presencia == 1 && entrada->uso == 0 && entrada->modificado == 0;
}

uint es_victima_clock_modificado_u(entradaTablaPaginas* entrada) {
	return entrada->presencia == 1 && entrada->uso == 0 && entrada->modificado == 1;
}

void actualizar_registros(entradaTablaPaginas* entrada, entradaTablaPaginas* entrada_victima) {
	// Limpieza de registro victima
	entrada_victima->presencia = 0;
	entrada_victima->uso = 0;
	// Revisar esto!!!
	// Lo actualizamos cuando venga la proxima vez que la carguemos
	/*
	if (entrada_victima->modificado == 1) {
		// Actualizar pagina en swap
		entrada_victima->modificado = 0;
	}
	*/

	// Carga de pagina solicitada
	// Le asigno el frame que fue desocupado y elegido como victima
	entrada->numeroMarco = entrada_victima->numeroMarco;
	entrada->uso = 1;
	entrada->presencia = 1;
}

int es_lista_circular_del_proceso(size_t pid, t_lista_circular* lista_circular) {
	return lista_circular->pid == pid;
}

t_lista_circular* obtener_lista_circular_del_proceso(size_t pid) {
	bool _es_lista_circular_del_proceso(void* elemento) {
		return es_lista_circular_del_proceso(pid, (t_lista_circular*) elemento);
	}
	return list_find(lista_frames_procesos, _es_lista_circular_del_proceso);
}

t_frame_lista_circular* obtener_elemento_lista_circular(t_lista_circular* lista, uint32_t numero_pagina) {
	int actualizacion_ok = 0;
	t_frame_lista_circular* frame_elemento_aux = lista->inicio;
	while (actualizacion_ok == 0) {
		if (frame_elemento_aux->info->numero_pagina == numero_pagina) {
			actualizacion_ok = 1;
		} else {
			frame_elemento_aux = frame_elemento_aux->sgte;
		}
	}
	return frame_elemento_aux;
}

/* No lo usamos
entradaTablaPaginas* obtener_entrada_tabla_de_paginas(uint32_t nro_tabla_paginas, entradaTablaPaginas* entrada) {
	pthread_mutex_lock(&mutex_lista_tablas_de_paginas);
	tablaDePaginas* tabla_paginas = list_get(listaTablaDePaginas, nro_tabla_paginas);
	pthread_mutex_unlock(&mutex_lista_tablas_de_paginas);

	return list_get(tabla_paginas->entradas, numero_pagina);
}
*/

/**************************************** FUNCIONES AUXILIARES LISTA CIRCULAR **********************************************/

t_lista_circular* list_create_circular() {
	t_lista_circular* lista = malloc(sizeof(t_lista_circular));;
    lista->inicio = NULL;
    lista->fin = NULL;
    lista->tamanio = 0;
    lista->puntero_algoritmo = NULL;
    return lista;
}

void insertar_lista_circular_vacia(t_lista_circular* lista, t_frame* frame) {
	t_frame_lista_circular* elemento_nuevo = malloc(sizeof(t_frame_lista_circular));
	elemento_nuevo->info = frame;
	elemento_nuevo->sgte = elemento_nuevo;
	lista->inicio = elemento_nuevo;
	lista->inicio->sgte = elemento_nuevo;
	lista->fin = elemento_nuevo;
	lista->tamanio=1;
	lista->puntero_algoritmo = elemento_nuevo;
	return;
}

void insertar_lista_circular(t_lista_circular* lista, t_frame* frame) {
	if (lista->tamanio == 0) {
		insertar_lista_circular_vacia(lista, frame);
	}
	else {
        t_frame_lista_circular *elemento_nuevo = malloc(sizeof(t_frame_lista_circular));
        elemento_nuevo->info = frame;
        elemento_nuevo->sgte = lista->inicio;
        lista->fin->sgte = elemento_nuevo;
        lista->fin = elemento_nuevo;
        lista->tamanio++;
    }
}










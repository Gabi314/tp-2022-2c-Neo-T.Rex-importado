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
pthread_mutex_t conexionKernel;
pthread_mutex_t conexionCpu;
pthread_mutex_t mutex_marcos_libres;
pthread_mutex_t mutex_lista_tablas_paginas;
pthread_mutex_t mutex_lista_entradas_tabla_paginas;

void* memoria;

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
	uint32_t valor = 0;
	for(int i=0; i< tamanioDeMemoria/sizeof(uint32_t); i++) {
		memcpy((memoria + sizeof(uint32_t) *i), &valor, sizeof(uint32_t));
	}
}

//Mandar el cont antes de inicializar estructuras a kernel
void inicializarEstructuras(int pid) {
	for(int i = 0; i < cantidadDeSegmentos; i++) {
		tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
		unaTablaDePaginas->pid = pid;
		unaTablaDePaginas->entradas = list_create();
		unaTablaDePaginas->numeroDeSegmento = i;
		cargarEntradasATabla(unaTablaDePaginas, i);

		pthread_mutex_lock(&mutex_lista_tablas_paginas);
		list_add(listaTablaDePaginas, unaTablaDePaginas);
		pthread_mutex_unlock(&mutex_lista_tablas_paginas);
		contNroTablaDePaginas++;
		log_info(logger, "PID: <%d> - Segmento: <%d> - TAMAÑO: <%d> paginas", pid, i, entradasPorTabla);
	}
}

void cargarEntradasATabla(tablaDePaginas* unaTablaDePaginas, int numeroDeSegmento) {
	for(int j=0; j < entradasPorTabla; j++){
		entradaTablaPaginas* unaEntradaDeTabla = malloc(sizeof(entradaTablaPaginas));

		unaEntradaDeTabla->numeroDeEntrada = j;
		unaEntradaDeTabla->numeroMarco = -1;
		unaEntradaDeTabla->presencia = 0;
		unaEntradaDeTabla->uso = 0;
		unaEntradaDeTabla->modificado = 0;
		unaEntradaDeTabla->posicionEnSwap = -1;
		unaEntradaDeTabla->numeroDeSegmento = numeroDeSegmento;

		pthread_mutex_lock(&mutex_lista_entradas_tabla_paginas);
		list_add(unaTablaDePaginas->entradas, unaEntradaDeTabla);
		pthread_mutex_unlock(&mutex_lista_entradas_tabla_paginas);
	}

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
}

marco* siguienteMarcoLibre() {
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0; i < list_size(listaDeMarcos); i++) {
		pthread_mutex_lock(&mutex_marcos_libres);
		unMarco = list_get(listaDeMarcos, i);
		pthread_mutex_unlock(&mutex_marcos_libres);

		if(unMarco->marcoLibre == 0) {
			return unMarco;
		}
	}
}

void modificarPaginaACargar(entradaTablaPaginas* unaEntrada, marco* marcoAASignar) {
	unaEntrada->numeroMarco = marcoAASignar->numeroDeMarco;
	unaEntrada->presencia = 1;
	unaEntrada->uso = 1;

	marcoAASignar->marcoLibre = 1;
}

marco* buscarMarco(int nroDeMarco) {
	marco* unMarco;
	for(int i=0; i < list_size(listaDeMarcos); i++) {
		pthread_mutex_lock(&mutex_marcos_libres);
		unMarco = list_get(listaDeMarcos, i);
		pthread_mutex_unlock(&mutex_marcos_libres);
		if(unMarco->numeroDeMarco == nroDeMarco) {
			return unMarco;
		}
	}
}

void liberarEspacioEnMemoria(tablaDePaginas* unaTablaDePaginas) {
	entradaTablaPaginas* unaEntrada;
	marco* marcoAsignado;
	t_list* listaDeEntradasAux = list_create();
	listaDeEntradasAux = unaTablaDePaginas->entradas;

	for(int j = 0; j < entradasPorTabla; j++) {
		pthread_mutex_lock(&mutex_lista_entradas_tabla_paginas);
		unaEntrada = list_get(listaDeEntradasAux, j);
		pthread_mutex_unlock(&mutex_lista_entradas_tabla_paginas);
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
}

void finalizacionDeProceso(int pid) {
	tablaDePaginas* unaTablaDePaginas;
	for(int i = 0; i < list_size(listaTablaDePaginas); i++) {
		pthread_mutex_lock(&mutex_lista_tablas_paginas);
		unaTablaDePaginas = list_get(listaTablaDePaginas, i);
		pthread_mutex_unlock(&mutex_lista_tablas_paginas);
		if(unaTablaDePaginas->pid == pid) {
			liberarEspacioEnMemoria(unaTablaDePaginas);
		}
	}
}

void cargarPagina(entradaTablaPaginas* unaEntrada,int pid) {
	if(unaEntrada->presencia == 0) {
		marco* marcoAAsignar;
		t_lista_circular* frames_proceso = obtener_lista_circular_del_proceso(pid);
		//Caso en el que se puede asignar un marco a un proceso de manera libre
		if(frames_proceso->tamanio < marcosPorProceso){
			marcoAAsignar = siguienteMarcoLibre();
			modificarPaginaACargar(unaEntrada, marcoAAsignar);
			insertar_lista_circular(frames_proceso,unaEntrada);
			if(unaEntrada->modificado == 1) {
				leerDeSwap(unaEntrada, unaEntrada->numeroMarco);
				unaEntrada->modificado = 0;

			}
		//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1
		}else{
			sustitucion_paginas(unaEntrada,pid);
			if(unaEntrada->modificado == 1){
				leerDeSwap(unaEntrada, unaEntrada->numeroMarco);
				unaEntrada->modificado = 0;
			}
		}
	}
}

void escribirElPedido(uint32_t datoAEscribir, int marco, int desplazamiento, int pidDeOperacion) {
	usleep(retardoMemoria*1000);
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy((memoria+posicion), &datoAEscribir, sizeof(uint32_t));

	log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d>", pidDeOperacion, posicion);
	actualizarBitUsoEntrada(marco, pidDeOperacion);
	log_info(logger,"paso e primero");
	actualizarBitModificadoEntrada(marco,pidDeOperacion);
}

void actualizarBitModificadoEntrada(int nroDeMarco, int pidDeOperacion) {
	t_lista_circular* frames_proceso;

	frames_proceso = obtener_lista_circular_del_proceso(pidDeOperacion);

	t_frame_lista_circular* elementoAEscribir = obtener_elemento_lista_circular_por_marco(frames_proceso,nroDeMarco);

	elementoAEscribir->info->modificado =  1;
}

void actualizarBitUsoEntrada(int nroDeMarco, int pidDeOperacion) {
	t_lista_circular* frames_proceso;

	frames_proceso = obtener_lista_circular_del_proceso(pidDeOperacion);
	t_frame_lista_circular* elementoAOperar = obtener_elemento_lista_circular_por_marco(frames_proceso,nroDeMarco);
	if(elementoAOperar->info->uso != 1){
		elementoAOperar->info->uso =  1;
	}
}

uint32_t leerElPedido(int marco, int desplazamiento,int pidOperacion) {
	usleep(retardoMemoria*1000);
	uint32_t datoALeer;
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy(&datoALeer,memoria+posicion, sizeof(uint32_t));
	log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección física: <%d>", pidActual, posicion);

	actualizarBitUsoEntrada(marco, pidOperacion);
	//if(datoALeer != 0) {
		return datoALeer;
	//}
}
void crearSwap() {
	remove(pathSwap);
	FILE* archivoSwap = fopen(pathSwap, "w+");
	truncate(pathSwap, tamanioSwap);

	for(int i=0; i<(tamanioSwap/sizeof(int));i++){
			fwrite("0",sizeof(int),1,archivoSwap);
	}

	fclose(archivoSwap);
}

void* obtenerPaginaDesdeEspacioUsuario(int marco) {
	usleep(retardoMemoria*1000);

	void* pagina = malloc(tamanioDePagina);

	int posicion = marco * tamanioDePagina;

	memcpy(pagina,(memoria+posicion),tamanioDePagina);

	return pagina;
}

void escribirEnSwap(entradaTablaPaginas* unaEntrada) {
	int numeroDeMarco = unaEntrada->numeroMarco;
	usleep(retardoSwap*1000);

	FILE* archivoSwap = fopen(pathSwap, "r+");

	if(unaEntrada->posicionEnSwap == -1) {
		unaEntrada->posicionEnSwap = posicionActualDeSwap;
		posicionActualDeSwap += tamanioDePagina;
	}
	void* paginaACargar = obtenerPaginaDesdeEspacioUsuario(numeroDeMarco);

//	uint32_t datoALeer;
//	memcpy(&datoALeer,paginaACargar, sizeof(uint32_t));
//	log_info(loggerAux,"Parte de la pagina a llevar a swap: %u",datoALeer);

	fseek(archivoSwap, unaEntrada->posicionEnSwap, SEEK_SET);
	fwrite((char*)paginaACargar,tamanioDePagina,1,archivoSwap);

	fclose(archivoSwap);

	log_info(logger, "SWAP OUT - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d>", pidActual,
			numeroDeMarco, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada);
}

void* obtenerPaginaDesdeSwap(int posicionEnSwap,FILE* archivoSwap) {
	usleep(retardoMemoria*1000);
	void* pagina = malloc(tamanioDePagina);
	//int posicion = marco * tamanioDePagina;
	memcpy(pagina, (archivoSwap+posicionEnSwap), tamanioDePagina);
	return pagina;
}

void leerDeSwap(entradaTablaPaginas* unaEntrada, int numeroDeMarcoNuevo) {
	usleep(retardoSwap*1000);

	void* pagina = malloc(tamanioDePagina);

	FILE* archivoSwap = fopen(pathSwap, "r");
	fseek(archivoSwap, unaEntrada->posicionEnSwap, SEEK_SET);

	fread(pagina,tamanioDePagina,1,archivoSwap);

//	uint32_t datoALeer;
//	memcpy(&datoALeer,pagina, sizeof(uint32_t));
//	log_info(loggerAux,"Parte de la pagina a traer de swap: %u",datoALeer);

	int posicion = numeroDeMarcoNuevo * tamanioDePagina;
	memcpy((memoria+posicion),pagina,tamanioDePagina);

	fclose(archivoSwap);
	log_info(logger, "SWAP IN - PID: <%d> - Marco: <%d> - Page In: <%d>|<%d>", pidActual,
			numeroDeMarcoNuevo, unaEntrada->numeroDeSegmento, unaEntrada->numeroDeEntrada);
}

/**************************************** ALGORITMOS DE SUSTITUCION **********************************************/

void sustitucion_paginas(entradaTablaPaginas* una_entrada, int pid) {
	t_lista_circular* frames_proceso = obtener_lista_circular_del_proceso(pid);

	if (strcmp("CLOCK", algoritmoDeReemplazo)==0) {
		algoritmo_clock(frames_proceso, una_entrada);
		return;
	}
	else if (strcmp("CLOCK-M", algoritmoDeReemplazo)==0) {
		algoritmo_clock_modificado(frames_proceso, una_entrada);
		return;
	}
	log_info(loggerAux, "Algoritmo de reemplazo invalido");
}

void algoritmo_clock(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {
	// Variables auxiliares
	t_frame_lista_circular* frame_puntero = malloc(sizeof(t_frame_lista_circular));
	entradaTablaPaginas* entrada_tabla_paginas_victima;
	uint hay_victima = 0;

	while(hay_victima == 0) {
		// Al implementar un puntero_algoritmo que se va desplazando dentro de la propia lista circular
		// 		lo reemplazamos por eso
		frame_puntero = frames_proceso->puntero_algoritmo;

		hay_victima = es_victima_clock(frame_puntero->info);

		if (hay_victima) {
			entrada_tabla_paginas_victima = frame_puntero->info;
			// Actualizo los registros REALES
			if(entrada_tabla_paginas_victima->modificado == 1){
				escribirEnSwap(entrada_tabla_paginas_victima);
			}
			actualizar_registros(entrada_tabla_paginas, frame_puntero->info);
			frame_puntero->info = entrada_tabla_paginas;

		} else {
			frame_puntero->info->uso = 0;
		}
		frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
	}
	log_info(logger,"REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
			pidActual,frame_puntero->info->numeroMarco,entrada_tabla_paginas_victima->numeroDeSegmento,entrada_tabla_paginas_victima->numeroDeEntrada,
			entrada_tabla_paginas->numeroDeSegmento,entrada_tabla_paginas->numeroDeEntrada);
}

void algoritmo_clock_modificado(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {
	// Variables auxiliares
	t_frame_lista_circular* frame_puntero = malloc(sizeof(t_frame_lista_circular));
	entradaTablaPaginas* entrada_tabla_paginas_victima;

	uint hay_victima_um = 0;
	uint hay_victima_u = 0;
	uint busquedas_um = 0;
	uint busquedas_u = 0;

	while (hay_victima_um == 0 && hay_victima_u == 0) {

		busquedas_um = 0;
		busquedas_u = 0;

		// Primeras iteraciones en busqueda de una pagina con U=0 && M=0
		// No se actualiza ningun bit
		while (hay_victima_um == 0 && busquedas_um < marcosPorProceso) {

			busquedas_um++;

			frame_puntero = frames_proceso->puntero_algoritmo;

			hay_victima_um = es_victima_clock_modificado_um(frame_puntero->info);

			if (hay_victima_um) {
				entrada_tabla_paginas_victima = frame_puntero->info;

				// Actualizo los registros REALES
				if(entrada_tabla_paginas_victima->modificado == 1){
					escribirEnSwap(entrada_tabla_paginas_victima);
				}

				actualizar_registros(entrada_tabla_paginas, frame_puntero->info);
				frame_puntero->info = entrada_tabla_paginas;

				frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
				break;
			}

			frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
		}

		// Si no hay victima de u=0 y m=0
		if (hay_victima_um == 0) {

			// Segundas iteraciones en busqueda de una pagina con U=0 && M=1
			// Se actualiza el bit de uso
			while (hay_victima_u == 0 && busquedas_u < marcosPorProceso) {

				busquedas_u++;

				frame_puntero = frames_proceso->puntero_algoritmo;

				hay_victima_u = es_victima_clock_modificado_u(frame_puntero->info);

				if (hay_victima_u) {
					entrada_tabla_paginas_victima = frame_puntero->info;

					// Actualizo los registros REALES
					if(entrada_tabla_paginas_victima->modificado == 1){
						escribirEnSwap(entrada_tabla_paginas_victima);
					}

					actualizar_registros(entrada_tabla_paginas, frame_puntero->info);
					frame_puntero->info = entrada_tabla_paginas;

					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
					break;
				} else {
					frame_puntero->info->uso = 0;
					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
				}
			}
		}
	}
	log_info(logger,"REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d>|<%d> - Page In: <%d>|<%d>",
				pidActual,frame_puntero->info->numeroMarco,entrada_tabla_paginas_victima->numeroDeSegmento,entrada_tabla_paginas_victima->numeroDeEntrada,
				entrada_tabla_paginas->numeroDeSegmento,entrada_tabla_paginas->numeroDeEntrada);
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
	// Actualizacion de la entrada deseada
	entrada->numeroMarco = entrada_victima->numeroMarco;
	entrada->uso = 1;
	entrada->presencia = 1;
}

int es_lista_circular_del_proceso(int pid, t_lista_circular* lista_circular) {
	return lista_circular->pid == pid;
}

t_lista_circular* obtener_lista_circular_del_proceso(int pid) {
	bool _es_lista_circular_del_proceso(void* elemento) {
		return es_lista_circular_del_proceso(pid, (t_lista_circular*) elemento);
	}
	return list_find(lista_frames_procesos, _es_lista_circular_del_proceso);
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

void list_create_circular(int pid) {
	t_lista_circular* lista = malloc(sizeof(t_lista_circular));;
    lista->inicio = NULL;
    lista->fin = NULL;
    lista->tamanio = 0;
    lista->puntero_algoritmo = NULL;
    lista->pid = pid;

    list_add(lista_frames_procesos,lista);
}

void insertar_lista_circular_vacia(t_lista_circular* lista, entradaTablaPaginas* entrada) {
	t_frame_lista_circular* elemento_nuevo = malloc(sizeof(t_frame_lista_circular));
	elemento_nuevo->info = entrada;
	elemento_nuevo->sgte = elemento_nuevo;
	lista->inicio = elemento_nuevo;
	lista->inicio->sgte = elemento_nuevo;
	lista->fin = elemento_nuevo;
	lista->tamanio=1;
	lista->puntero_algoritmo = elemento_nuevo;
	return;
}

void insertar_lista_circular(t_lista_circular* lista, entradaTablaPaginas* entrada) {
	if (lista->tamanio == 0) {
		insertar_lista_circular_vacia(lista, entrada);
	}
	else {
        t_frame_lista_circular *elemento_nuevo = malloc(sizeof(t_frame_lista_circular));
        elemento_nuevo->info = entrada;
        elemento_nuevo->sgte = lista->inicio;
        lista->fin->sgte = elemento_nuevo;
        lista->fin = elemento_nuevo;
        lista->tamanio++;
    }
}

t_frame_lista_circular* obtener_elemento_lista_circular_por_marco(t_lista_circular* lista, uint32_t numero_marco) {
	int actualizacion_ok = 0;
	t_frame_lista_circular* frame_elemento_aux = lista->inicio;
	while (actualizacion_ok == 0) {
		if (frame_elemento_aux->info->numeroMarco == numero_marco) {
			actualizacion_ok = 1;
		} else {
			log_info(logger,"estoy dentro de else, buscando marco <%u> y tengo el marco <%d>",numero_marco,frame_elemento_aux->info->numeroMarco );
			frame_elemento_aux = frame_elemento_aux->sgte;
		}
	}


	return frame_elemento_aux;
}

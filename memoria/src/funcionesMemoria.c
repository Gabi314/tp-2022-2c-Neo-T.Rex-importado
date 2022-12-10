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
	for(int j=0; j < entradasPorTabla; j++){
		entradaTablaPaginas* unaEntradaDeTabla = malloc(sizeof(entradaTablaPaginas));

		unaEntradaDeTabla->numeroDeEntrada = j;
		unaEntradaDeTabla->numeroMarco = -1;
		unaEntradaDeTabla->presencia = 0;
		unaEntradaDeTabla->uso = 0;
		unaEntradaDeTabla->modificado = 0;
		unaEntradaDeTabla->posicionEnSwap = -1;
		unaEntradaDeTabla->numeroDeSegmento = numeroDeSegmento;

		list_add(unaTablaDePaginas->entradas, unaEntradaDeTabla);
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
	//unaEntrada->numeroMarco = -1;
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

void modificarPaginaACargar(entradaTablaPaginas* unaEntrada, marco* marcoAASignar) {
	unaEntrada->numeroMarco = marcoAASignar->numeroDeMarco;
	unaEntrada->presencia = 1;
	unaEntrada->uso = 1;

	marcoAASignar->marcoLibre = 1;
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


void escribirElPedido(uint32_t datoAEscribir, int marco, int desplazamiento) {
	usleep(retardoMemoria*1000);
	int posicion = marco * tamanioDePagina + desplazamiento;
	memcpy(&memoria+posicion, &datoAEscribir, sizeof(uint32_t));

	log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d>", pidActual, posicion);

	entradaTablaPaginas* entradaAEscribir = entradaCargadaConMarcoAsignado(marco);
	entradaAEscribir->modificado = 1;

	//enviar_mensaje("Se escribio el valor correctamente",clienteCpu);
}

//ESTA DEBERIA RECORRER LA LISTACIRCULAR Y DEVOLVER LA ENTRADA CARGADA CON ESE MARCO
entradaTablaPaginas* entradaCargadaConMarcoAsignado(int nroDeMarco) {
	t_lista_circular* frames_proceso;
	entradaTablaPaginas* entradaAEscribir;
	for(int i=0;i<list_size(lista_frames_procesos);i++){
		//PROBLEMA
		frames_proceso = obtener_lista_circular_del_proceso(i);

		for(int j=0;i<frames_proceso->tamanio;j++){
			entradaAEscribir = frames_proceso->puntero_algoritmo->info;
			if(entradaAEscribir->numeroMarco == nroDeMarco){
				return entradaAEscribir;
			}else {
				frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
			}
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

uint32_t sustitucion_paginas(entradaTablaPaginas* una_entrada, size_t pid) {
	t_lista_circular* frames_proceso = obtener_lista_circular_del_proceso(pid);

	if (strcmp("CLOCK", algoritmoDeReemplazo)==0) {
		algoritmo_clock(frames_proceso, una_entrada);
	}
	else if (strcmp("CLOCK-M", algoritmoDeReemplazo)==0) {
		algoritmo_clock_modificado(frames_proceso, una_entrada);
	}
	log_info(loggerAux, "Algoritmo de reemplazo invalido");
	return -1;
}
//uint32_t algoritmo_clock(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {//CREO QUE NO ES NECESARIO DEVOLVER NADA DE ACA
void algoritmo_clock(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {
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
	//return frame_puntero->info->numeroMarco;
}

//uint32_t algoritmo_clock_modificado(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {//CREO QUE NO ES NECESARIO DEVOLVER NADA DE ACA
void algoritmo_clock_modificado(t_lista_circular* frames_proceso, entradaTablaPaginas* entrada_tabla_paginas) {
	//t_registro_segundo_nivel* registro_segundo_nivel = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, numero_pagina);
	// Variables auxiliares
	t_frame_lista_circular* frame_puntero = malloc(sizeof(t_frame_lista_circular));
	//t_registro_segundo_nivel* registro_segundo_nivel_victima = malloc(sizeof(t_registro_segundo_nivel));;
	//t_registro_segundo_nivel* registro_segundo_nivel_actualizado = malloc(sizeof(t_registro_segundo_nivel));

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
			//	registro_segundo_nivel_victima = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);

				actualizar_registros(entrada_tabla_paginas, frame_puntero->info);

			//	frame_puntero->info->numero_pagina = numero_pagina;
				//frame_puntero->info->uso = 1;

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
					//registro_segundo_nivel_victima = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);

					actualizar_registros(entrada_tabla_paginas, frame_puntero->info);

					//frame_puntero->info->numero_pagina = numero_pagina;
					//frame_puntero->info->uso = 1;

					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
					break;
				} else {
					//registro_segundo_nivel_actualizado = obtener_registro_segundo_nivel(numero_tabla_primer_nivel, frame_puntero->info->numero_pagina);
					//registro_segundo_nivel_actualizado->uso = 0;

					//CREO QUE ESTO ES CUANDO ESTA TERMINANDO EL ALGORITMO QUE PONE EN 0 LOS USOS
					frame_puntero->info->uso = 0;
					frames_proceso->puntero_algoritmo = frames_proceso->puntero_algoritmo->sgte;
				}
			}
		}
	}
	//return frame_puntero->info->numeroDeEntrada;
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
		if (frame_elemento_aux->info->numeroDeEntrada == numero_pagina) {
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

void list_create_circular(int pid) {
	t_lista_circular* lista = malloc(sizeof(t_lista_circular));;
    lista->inicio = NULL;
    lista->fin = NULL;
    lista->tamanio = 0;
    lista->puntero_algoritmo = NULL;
    lista->pid = pid;

    list_add(lista_frames_procesos,lista);
    //return lista; ANTES ERA DE TIPO LISTA PERO NO ME DEBERIA DEVOLVER, PORQUE HACE TODO ACA
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










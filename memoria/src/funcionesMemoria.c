#include "funcionesMemoria.h"

//--------------------------DECLARO VARIABLES
t_log* logger;

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

//Variables a utilizar
void* memoria; // espacio de usuario de la memoria

int contadorDeEntradasPorProceso;
t_list* listaDeMarcos;
t_list* listaDeEntradasEnMemoria;
//CREO QUE NO SE UTILIZA t_list* listaDePaginasEnMemoria;
t_list* listaTablaDePaginas;

int posicionDelPuntero;
int contadorDeMarcosPorProceso = 0;
int cantidadDeSegmentos;

int contNroTablaDePaginas = 0;

//------------------------- DEFINICION DE FUNCIONES
int chequeoCantidadArchivos(int argc){
	if(argc < 2) {
		    log_error(logger,"Falta un parametro");
		    return EXIT_FAILURE;
		}
	return EXIT_SUCCESS;
}

void crearConfiguraciones(char* unaConfig){
	t_config* config = config_create(unaConfig);

	if(config  == NULL){
		printf("Error leyendo archivo de configuración. \n");
	}

	tamanioDeMemoria = config_get_int_value(config,"TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config,"TAM_PAGINA");

	entradasPorTabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config,"RETARDO_SWAP");
	pathSwap = config_get_string_value(config,"PATH_SWAP");
	tamanioSwap = config_get_int_value(config,"TAMANIO_SWAP");

}

void inicializarMemoria(){
	memoria = malloc(tamanioDeMemoria); // inicializo el espacio de usuario en memoria
}
//Mandar el cont antes de inicializar estructuras a kernel
void inicializarEstructuras(int pid){
	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	for(int i = 0; i < cantidadDeSegmentos; i++){
		unaTablaDePaginas->pid = pid;
		unaTablaDePaginas->entradas = list_create();

		cargarEntradasATabla(unaTablaDePaginas);

		//DEJO ESTA LISTA PARA "INICIALIZAR PROCESO", PORQUE DESPUES HAY QUE VER DONDE SE AGREGO ESTA TABLA DE PAGINAS Y MANDARLA A CPU
		list_add(listaTablaDePaginas,unaTablaDePaginas);
		contNroTablaDePaginas++;
	}

}


void cargarEntradasATabla(tablaDePaginas* unaTablaDePaginas){

	for(int j=0;j<entradasPorTabla;j++){ //crear x cantidad de marcos y agregar de a 1 a la lista
		entradaTablaPaginas* unaEntradaDeTabla = malloc(sizeof(entradaTablaPaginas));

		unaEntradaDeTabla->numeroDeEntrada = j; //Antes era igual a contadorDeEntradasPorProceso, pero como ahora es 1 sola tabla, solo tiene la cantidad = entradasPorTabla
		unaEntradaDeTabla->numeroMarco = -1;
		unaEntradaDeTabla->presencia = 0;
		unaEntradaDeTabla->uso = 0;
		unaEntradaDeTabla->modificado = 0;
		unaEntradaDeTabla->posicionEnSwap = -1;

		//contadorDeEntradasPorProceso++;

		list_add(unaTablaDePaginas->entradas,unaEntradaDeTabla);

		}
	//contadorDeEntradasPorProceso = 0;
}

void inicializarMarcos(){

	for(int j=0;j<(tamanioDeMemoria/tamanioDePagina);j++){
		marco* unMarco = malloc(sizeof(marco));
		unMarco->numeroDeMarco = j;
		unMarco->marcoLibre = 0;
		list_add(listaDeMarcos,unMarco);
	}
}

int posicionDePunteroDelAlgoritmo(int i){
	posicionDelPuntero += i;

	if(posicionDelPuntero<marcosPorProceso){
		return posicionDelPuntero;
	}else{
		posicionDelPuntero = 0;
		return	posicionDelPuntero;
	}
}

void sacarMarcoAPagina(entradaTablaPaginas* unaEntrada){
	unaEntrada->presencia = 0;
	unaEntrada->uso = 0;
	//unaEntrada->numeroMarco = -1;

}

void reemplazarTodosLosUsoACero(t_list* listaDeEntradasEnMemoria){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		unaEntrada = list_get(listaDeEntradasEnMemoria,i);
		unaEntrada->uso = 0;
			}
}


//FUNCIONES DE LOS 2 ALGORITMOS --> TENER EN CUENTA QUE PASO LA LISTA DE ENTRADAS
int algoritmoClock(t_list* listaDeEntradasEnMemoria){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0");
	log_info(logger,"Por algoritmo reemplazo todos los bit de uso a 0 y busco de nuevo");
	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
	reemplazarTodosLosUsoACero(listaDeEntradasEnMemoria);

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

}

int algoritmoClockM (t_list* listaDeEntradasEnMemoria){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 1");
	log_info(logger,"Reemplazo todos los bit de uso en 0");
	reemplazarTodosLosUsoACero(listaDeEntradasEnMemoria);
	log_info(logger,"Busco con bit de uso en 0 y modificado en 0 ");

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasEnMemoria,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
}

marco* siguienteMarcoLibre(){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){

		unMarco = list_get(listaDeMarcos,i);

		if(unMarco->marcoLibre == 0){
			return unMarco;
		}
	}
}

void modificarPaginaACargar(entradaTablaPaginas* unaEntrada, int nroDeMarcoAASignar){
	unaEntrada->numeroMarco = nroDeMarcoAASignar;
	unaEntrada->presencia = 1;
	unaEntrada->uso = 1;
	//unaPagina->modificado = 0; ANALIZAR CASO DE MODIFICADO EN 1
	marco* marcoAsignado = buscarMarco(nroDeMarcoAASignar);
	marcoAsignado->marcoLibre = 1;
}

marco* buscarMarco(int nroDeMarco){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){
		unMarco = list_get(listaDeMarcos,i);
		if(unMarco->numeroDeMarco == nroDeMarco){
			return unMarco;
		}
	}
	free(unMarco);
}

int indiceDeEntradaAReemplazar(int numeroDeMarco){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	for(int i=0;i < list_size(listaDeEntradasEnMemoria);i++){
		unaEntrada=list_get(listaDeEntradasEnMemoria,i);
		if(unaEntrada->numeroMarco == numeroDeMarco){
			return i;
		}
	}
}

void liberarEspacioEnMemoria(tablaDePaginas* unaTablaDePaginas){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	marco* marcoAsignado = malloc(sizeof(marco));
	t_list* listaDeEntradasAux = list_create();
	listaDeEntradasAux = unaTablaDePaginas->entradas;

	for(int j = 0;j<entradasPorTabla;j++){
		unaEntrada = list_get(listaDeEntradasAux,j);
		if(unaEntrada->presencia == 1 && unaEntrada->modificado ==0){
			sacarMarcoAPagina(unaEntrada);
			marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
			marcoAsignado->marcoLibre=0;
		}
		if(unaEntrada->presencia == 1 && unaEntrada->modificado ==1){
			//NO SE QUE TAN UTIL PUEDE SER ESCRIBIR EN SWAP CUANDO SE LIBERA LA MEMORIA PORQUE EN ESTE TP NO HAY SUSPENSION DE PROCESO
			escribirEnSwap(unaEntrada);

			sacarMarcoAPagina(unaEntrada);
			marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
			marcoAsignado->marcoLibre=0;
		}
	}
	list_clean(listaDeEntradasEnMemoria);
}

void finalizacionDeProceso(int pid){
	int nroTablaDePaginas = buscarNroTablaDePaginas(pid);

	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	unaTablaDePaginas = list_get(listaTablaDePaginas,nroTablaDePaginas);
	liberarEspacioEnMemoria(unaTablaDePaginas);

//Para eliminar espacio ocupado en SWAP no es necesario que borre algo en el archivo ni nada porque con escribirSwap, se sobreescribe, asi que si alguno ocupa el lugar lo borra
}

int buscarNroTablaDePaginas(int pid){
	tablaDePaginas* unaTablaDe1erNivel = malloc(sizeof(tablaDePaginas));

	for(int i=0;i < list_size(listaTablaDePaginas);i++){

		unaTablaDe1erNivel = list_get(listaTablaDePaginas,i);

		if(unaTablaDe1erNivel->pid == pid){
			return i;
		}
	}
	return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe1erNivel);
}

//Funcion de cargar una pagina, por ahora la hago global y en 1 proceso porque no se como funciona
void cargarPagina(entradaTablaPaginas* unaEntrada){
	marco* marcoAAsignar;
	int numeroMarcoPrevio = unaEntrada->numeroMarco;

	//Caso en el que se puede asignar un marco a un proceso de manera libre
	if(contadorDeMarcosPorProceso<marcosPorProceso && (list_size(listaDeEntradasEnMemoria)<=(tamanioDeMemoria/tamanioDePagina))){
		marcoAAsignar = siguienteMarcoLibre();
		modificarPaginaACargar(unaEntrada,marcoAAsignar->numeroDeMarco);
		list_add(listaDeEntradasEnMemoria,unaEntrada);
		contadorDeMarcosPorProceso++; // ANALIZAR CONTADOR POR MULTIPROCESAMIENTO
		if(unaEntrada->modificado == 1){
			leerDeSwap(numeroMarcoPrevio,unaEntrada->numeroMarco);
		}
		log_info(logger,"La cantidad de marcos asignados a este proceso es: %d", contadorDeMarcosPorProceso);
		//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1
	}else if(strcmp(algoritmoDeReemplazo,"CLOCK") == 0){
			int marcoAAsignar = algoritmoClock(listaDeEntradasEnMemoria);
			modificarPaginaACargar(unaEntrada,marcoAAsignar);
			int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			list_replace(listaDeEntradasEnMemoria, posicionAReemplazar, unaEntrada);

			if(unaEntrada->modificado == 1){
				leerDeSwap(numeroMarcoPrevio,unaEntrada->numeroMarco);
			}

	}else if(strcmp(algoritmoDeReemplazo,"CLOCK M") == 0){
		int marcoAAsignar = algoritmoClockM(listaDeEntradasEnMemoria);
		modificarPaginaACargar(unaEntrada,marcoAAsignar);
		int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
		list_replace(listaDeEntradasEnMemoria, posicionAReemplazar, unaEntrada);

		if(unaEntrada->modificado == 1){
			leerDeSwap(numeroMarcoPrevio,unaEntrada->numeroMarco);
		}
	}
}

void escribirElPedido(uint32_t datoAEscribir,int marco,int desplazamiento){
	usleep(retardoMemoria);
	int posicionDeDatoAEscribir = marco * tamanioDePagina + sizeof(uint32_t)*desplazamiento;
	memcpy(&memoria+posicionDeDatoAEscribir, &datoAEscribir, sizeof(uint32_t));

	entradaTablaPaginas* entradaAEscribir = entradaCargadaConMarcoAsignado(marco);
	entradaAEscribir->modificado = 1;
	//enviar_mensaje("Se escribio el valor correctamente",clienteCpu);
}

entradaTablaPaginas* entradaCargadaConMarcoAsignado(int nroDemarco){
	entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
	for(int i = 0; i<list_size(listaDeEntradasEnMemoria);i++){
		unaEntrada = list_get(listaDeEntradasEnMemoria,i);

		if(unaEntrada->numeroMarco == nroDemarco){
			return unaEntrada;
		}
	}
}

uint32_t leerElPedido(int marco,int desplazamiento){
	usleep(retardoMemoria);
	uint32_t datoALeer;
	int posicion = marco * tamanioDePagina + sizeof(uint32_t)*desplazamiento;
	memcpy(&datoALeer,&memoria+posicion,sizeof(uint32_t));

	if(datoALeer != 0){
		return datoALeer;
	}
}

void crearSwap(){
	//char* nombrePathCompleto = nombreArchivoProceso(pid);
	remove(pathSwap);
	FILE* archivoSwap = fopen(pathSwap, "w+");
	truncate(pathSwap,tamanioSwap);
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

void escribirEnSwap(entradaTablaPaginas* unaEntrada){
	int numeroDeMarco = unaEntrada->numeroMarco;
	usleep(retardoSwap);
	FILE* archivoSwap = fopen(pathSwap, "r+");

	int posicionDeLaPaginaEnSwapInicial = tamanioDePagina * numeroDeMarco; //Aca modifique el numero de pagina por numero de marco
	unaEntrada->posicionEnSwap = posicionDeLaPaginaEnSwapInicial;
	for(int i=0; i<(tamanioDePagina/sizeof(uint32_t));i++){
		fseek(archivoSwap, posicionDeLaPaginaEnSwapInicial, SEEK_SET);
		fseek(archivoSwap, i*4, SEEK_CUR);

		uint32_t datoAEscribir = leerElPedido(numeroDeMarco,i);
		char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
		fputs(datoAEscribirEnChar,archivoSwap);
	}

	fclose(archivoSwap);
}

void leerDeSwap(int numeroDeMarcoPrevio,int numeroDeMarcoNuevo){
	usleep(retardoSwap);

	char* parteDePagina = string_new();
	FILE* archivoSwap = fopen(pathSwap, "r");

	int posicionDeLaPaginaALeer = tamanioDePagina * numeroDeMarcoPrevio;

	for(int i = 0; i<entradasPorTabla*entradasPorTabla;i++){
		fseek(archivoSwap, posicionDeLaPaginaALeer, SEEK_SET);
		fseek(archivoSwap, i*4, SEEK_CUR);

		fgets(parteDePagina,sizeof(uint32_t)+1,archivoSwap);

		uint32_t parteDePaginaEnInt = atoi(parteDePagina);

		memcpy(&memoria+(tamanioDePagina*numeroDeMarcoNuevo)+i*sizeof(uint32_t),&parteDePaginaEnInt, sizeof(uint32_t));
	}
	fclose(archivoSwap);
}

void suspensionDeProceso(int pid){
	int nroTablaDePaginas = buscarNroTablaDePaginas(pid);
	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	unaTablaDePaginas = list_get(listaTablaDePaginas,nroTablaDePaginas);
	liberarEspacioEnMemoria(unaTablaDePaginas);

}

/* COMENTARIOS:
 * SIEMPRE VOY A LEER DE ESPACIO DE USUARIO->POR ESO SIEMPRE TENGO QUE TENER CARGADO EN MEMORIA (VOID*)
 *
 * CADA ACCESO A TABLA DE PAGINAS/ESPACIO DE USUARIO (VOID) TENDRA ESPERA DEFINIDO POR ARCHIVO DE CONFIGURACION
 *
 * LA ESCRITURA EN SWAP SE HARA CUANDO HAYA PF PORQUE NO HAY SUSPENSION DE PROCESO -> LA FUNCION DE ESCRIBIR EN SWAP SE PONE EN LOS ALGORITMOS CUANDO SE REEMPALAZA UNO MODIFICADO
 * TAMBIEN SE TIENE QUE LEER DE SWAP CUANDO SE CARGUE UNA PAGINA QUE ESTABA MODIFICADA Y CARGARLA A MEMORIA PRINCIPAL, si estaba modificada y la recupero puedo hacer que ponga
 * el modificado en 0, y despues siga haciendo lo que tiene que hacer
 * TODOS LOS ACCESOS A SWAP DEBEN TENER RETARDO, ES DECIR TODAS LAS OPERACIONES DE LECTURA Y ESCRITURA
 *
 *
 *
 *
 *
 * */



//POSIBLEMENTE SEAN MUY PARECIDAS A LAS FUNCIONES QUE SE USARON EN EL TP PASADO
/*


void copiar(int marcoDeDestino,int desplazamientoDestino,int marcoDeOrigen,int desplazamientoOrigen){
	uint32_t datoACopiar;
	int posicionACopiar = marcoDeDestino * tamanioDePagina + sizeof(uint32_t)*desplazamientoDestino;
	int posicionDondeSeCopia = marcoDeOrigen * tamanioDePagina + sizeof(uint32_t)*desplazamientoOrigen;

	memcpy(&datoACopiar,&memoria+posicionDondeSeCopia,sizeof(uint32_t));
	memcpy(&memoria+posicionACopiar,&datoACopiar,sizeof(uint32_t));
}




void escribirEnSwap(int numeroDeMarco,int pid,int numeroDePagina){

	char* nombreDelArchivo = nombreArchivoProceso(pid);
	FILE* archivoSwap = fopen(nombreDelArchivo, "r+");

	int posicionDeLaPaginaEnSwapInicial = (entradasPorTabla*(tamanioDePagina/sizeof(int))*numeroDePagina) + numeroDePagina;

	for(int i=0; i<(tamanioDePagina/sizeof(uint32_t));i++){
	fseek(archivoSwap, posicionDeLaPaginaEnSwapInicial, SEEK_SET);
	//int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina;
	fseek(archivoSwap, i*4, SEEK_CUR);

	int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina + i*sizeof(uint32_t);
	uint32_t datoAEscribir = leerElPedido(numeroDeMarco,posicionDeValorEnMemoria);//Aca le agregue el numero de marco como primer parametro, revisar si esta bien
	char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
	fputs(datoAEscribirEnChar,archivoSwap);

	}

	fclose(archivoSwap);
}

void leerDeSwap(int numeroDePagina,int numeroDeMarco){
	char* parteDePagina = string_new();

	char* nombreDelArchivo = nombreArchivoProceso(pidActual); // capaz hay que pasarselo
	FILE* archivoSwap = fopen(nombreDelArchivo, "r");

	int posicionDeLaPaginaALeer = (entradasPorTabla*(tamanioDePagina/sizeof(int))*numeroDePagina) + numeroDePagina;


	for(int i = 0; i<entradasPorTabla*entradasPorTabla;i++){
	fseek(archivoSwap, posicionDeLaPaginaALeer, SEEK_SET);
	fseek(archivoSwap, i*4, SEEK_CUR);

	fgets(parteDePagina,sizeof(uint32_t)+1,archivoSwap);

	uint32_t parteDePaginaEnInt = atoi(parteDePagina);

	memcpy(&memoria+(tamanioDePagina*numeroDeMarco)+i*sizeof(uint32_t),&parteDePaginaEnInt, sizeof(uint32_t));
//	log_info(logger,parteDePagina);

	}
}

void crearDirectorio(){

	errno = 0;
	int ret = mkdir(pathSwap,S_IRWXU);
	if (ret == -1) {
		switch (errno) {
			case EACCES :
				log_info(logger,"No permite escribir");
				break;
			case EEXIST:
				log_info(logger,"Ya existe la direccion de swap");
				break;
			}
	}
}


*/














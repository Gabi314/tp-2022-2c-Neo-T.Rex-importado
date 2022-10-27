#ifndef FUNCIONES_MEMORIA_H_
#define FUNCIONES_MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<string.h>
#include<commons/string.h>
#include<assert.h>
#include<stdbool.h>

#include<pthread.h>

#include <errno.h>

#define IP_MEMORIA "127.0.0.1"
#define PUERTO_MEMORIA 8002

//--------------------------DECLARO STRUCTS
typedef struct{
	int numeroDeEntrada;
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;
	int posicionEnSwap;
}entradaTablaPaginas;

typedef struct{// capaz usar diccionario
	t_list* entradas;
	int pid;
}tablaDePaginas;

typedef struct{
	int numeroDeMarco;//Hacer lista de marcos, el indice en la lista es el numero de marco, la cantidad de marcos es
	int marcoLibre;// tam memoria/tam pagina
}marco;

//--------------------------DECLARO VARIABLES

extern t_log* logger;
//Variables globales de config
extern int tamanioDeMemoria;
extern int tamanioDePagina;
extern int entradasPorTabla;
extern int retardoMemoria;
extern char* algoritmoDeReemplazo;
extern int marcosPorProceso;
extern int retardoSwap;
extern char* pathSwap;
extern int tamanioSwap;

//Variables a utilizar
extern void* memoria; // espacio de usuario de la memoria
extern int contadorDeEntradasPorProceso;
extern t_list* listaDeMarcos;
extern t_list* listaDeEntradasEnMemoria;
extern t_list* listaTablaDePaginas;
extern t_list* listaDePaginasEnMemoria;
extern int posicionDelPuntero;
extern int contadorDeMarcosPorProceso;

//-------------------------DECLARO FUNCIONES
int chequeoCantidadArchivos(int);
void crearConfiguraciones(char*);
void crearDirectorio();

void inicializarMemoria();
void inicializarEstructuras(int);
void inicializarMarcos();
void cargarEntradasATabla(tablaDePaginas*);

int posicionDePunteroDelAlgoritmo(int);
void sacarMarcoAPagina(entradaTablaPaginas*);
void reemplazarTodosLosUsoACero(t_list*);
int algoritmoClock(t_list*);
int algoritmoClockM (t_list*);
marco* siguienteMarcoLibre();
marco* buscarMarco(int);
int indiceDeEntradaAReemplazar(int);
void finalizacionDeProceso(int);
int buscarNroTablaDePaginas(int);
void modificarPaginaACargar(entradaTablaPaginas*, int);
void cargarPagina(entradaTablaPaginas*);
void liberarEspacioEnMemoria(tablaDePaginas*);





#endif /* FUNCIONES_MEMORIA_H_*/

#ifndef FUNCIONES_MEMORIA_H_
#define FUNCIONES_MEMORIA_H_

#include <sharedUtils.h>

#include <stdbool.h>
#include <errno.h>
#include <unistd.h> //Para el truncate
#include <inttypes.h>

#define IP_MEMORIA "127.0.0.1"

/* *** STRUCTS *** */

typedef struct {
	int numeroDeEntrada;
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;
	int posicionEnSwap;
	int numeroDeSegmento;
} entradaTablaPaginas;

typedef struct{
	int pid;
	int posicionDelPuntero;
} entradaListaPunteros;

typedef struct {// capaz usar diccionario
	t_list* entradas;
	int pid;
	int numeroDeSegmento;
} tablaDePaginas;

typedef struct {
	int numeroDeMarco;//Hacer lista de marcos, el indice en la lista es el numero de marco, la cantidad de marcos es
	int marcoLibre;// tam memoria/tam pagina
} marco;

/* *** STRUCTS ALGORITMOS SUSTITUCION *** */

typedef struct {
	uint32_t numero_frame;
	uint32_t numero_pagina;
	uint uso;
	uint modificado;
	uint presencia;
} t_frame;

typedef struct t_frame_lista_circular {
	//t_frame* info;
	entradaTablaPaginas* info;
	struct t_frame_lista_circular* sgte;
} t_frame_lista_circular;

typedef struct {
	t_frame_lista_circular* inicio;
	t_frame_lista_circular* fin;
	int tamanio;
	t_frame_lista_circular* puntero_algoritmo;
	size_t pid;
} t_lista_circular;

/* *** VARIABLES *** */

extern t_log* logger;
extern t_log* loggerAux;
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

extern t_list* listaDePunterosYPids;
extern int posicionDelPuntero;
extern int contadorDeMarcosPorProceso;
extern int cantidadDeSegmentos;

extern int flagDeEntradasPorTabla;
//extern int marco;
extern int desplazamiento;

extern int posicionActualDeSwap;

extern int contNroTablaDePaginas;
extern int clienteCpu;
extern int clienteKernel;
extern char* ipMemoria;
extern char* puertoMemoria;

extern int pidActual;

//Variables de conexiones
extern int clienteCpu;

/* *** FUNCIONES *** */
void funcionMain(int, char**);
int chequeoCantidadArchivos(int);
void crearConfiguraciones(char*);
void crearDirectorio();

void inicializarMemoria();
void inicializarEstructuras(int);
void inicializarMarcos();
void cargarEntradasATabla(tablaDePaginas*,int);

int posicionDePunteroDelAlgoritmo(int);
void sacarMarcoAPagina(entradaTablaPaginas*);
void reemplazarTodosLosUsoACero(t_list*);
int algoritmoClock(t_list*,entradaTablaPaginas*,int);
int algoritmoClockM (t_list*,entradaTablaPaginas*,int);
marco* siguienteMarcoLibre();
marco* buscarMarco(int);
int indiceDeEntradaAReemplazar(int);
void finalizacionDeProceso(int);
int buscarNroTablaDePaginas(int);
void modificarPaginaACargar(entradaTablaPaginas*, int);
void cargarPagina(entradaTablaPaginas*,int);
void liberarEspacioEnMemoria(tablaDePaginas*);

void enviarTamanioDePaginaYCantidadDeEntradas(int);

void escribirElPedido(uint32_t,int, int);
entradaTablaPaginas* entradaCargadaConMarcoAsignado(int);
uint32_t leerElPedido(int, int);
uint32_t leerEnSwap(int,int);

void crearSwap();
void escribirEnSwap(entradaTablaPaginas*);
entradaTablaPaginas* entradaCargadaConMarcoAsignado(int);
void leerDeSwap(entradaTablaPaginas*,int);
void suspensionDeProceso(int);

/* *** FUNCIONES DE CONEXIONES *** */
int conexionConCpu(void*);
int conexionConKernel(void*);
int marcoSegunIndice(int, int);
void chequeoDeIndice(int);
int server_escuchar(t_log*, char*, char*, int);

/* *** FUNCIONES DE ALGORITMOS *** */
t_lista_circular* list_create_circular();
void insertar_lista_circular_vacia(t_lista_circular*, t_frame*);
void insertar_lista_circular(t_lista_circular*, t_frame*);
uint es_victima_clock(t_frame*);
uint es_victima_clock_modificado_um(t_frame*);
uint es_victima_clock_modificado_u(t_frame*);
uint32_t algoritmo_clock(t_lista_circular*, uint32_t, uint32_t);
uint32_t algoritmo_clock_modificado(t_lista_circular*, uint32_t, uint32_t);

#endif /* FUNCIONES_MEMORIA_H_*/

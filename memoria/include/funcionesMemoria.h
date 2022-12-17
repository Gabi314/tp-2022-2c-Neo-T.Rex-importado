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

typedef struct {
	t_list* entradas;
	int pid;
	int numeroDeSegmento;
} tablaDePaginas;

typedef struct {
	int numeroDeMarco;
	int marcoLibre;
} marco;

/* *** STRUCTS ALGORITMOS SUSTITUCION *** */

/*
typedef struct {
	uint32_t numero_frame;
	uint32_t numero_pagina;
	uint32_t numero_segmento;
	uint uso;
	uint modificado;
	uint presencia;
} t_frame;
*/

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
	int pid;
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
extern t_list* listaDeMarcos;
extern t_list* listaTablaDePaginas;
extern t_list* lista_frames_procesos;

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

extern pthread_mutex_t conexionKernel;
extern pthread_mutex_t conexionCpu;
extern pthread_mutex_t listaMarcos;
extern pthread_mutex_t mutex_lista_tablas_paginas;
extern pthread_mutex_t mutex_lista_entradas_tabla_paginas;

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

void sacarMarcoAPagina(entradaTablaPaginas*);
marco* siguienteMarcoLibre();
marco* buscarMarco(int);
void finalizacionDeProceso(int);
void modificarPaginaACargar(entradaTablaPaginas*,marco*);
void cargarPagina(entradaTablaPaginas*,int);
void liberarEspacioEnMemoria(tablaDePaginas*);

void enviarTamanioDePaginaYCantidadDeEntradas(int);

void escribirElPedido(uint32_t,int, int,int);
t_frame_lista_circular* obtener_elemento_lista_circular_por_marco(t_lista_circular*, uint32_t);
void actualizarBitModificadoEntrada(int,int);
void actualizarBitUsoEntrada(int,int);
uint32_t leerElPedido(int, int,int);
uint32_t leerEnSwap(int,int);

void crearSwap();
void escribirEnSwap(entradaTablaPaginas*);
void leerDeSwap(entradaTablaPaginas*,int);

/* *** FUNCIONES DE CONEXIONES *** */
int conexionConCpu(void*);
int conexionConKernel(void*);
int marcoSegunIndice(int, int);
void chequeoDeIndice(int);
int server_escuchar(t_log*, char*, char*, int);

/* *** FUNCIONES DE ALGORITMOS *** */
void list_create_circular(int);
void insertar_lista_circular_vacia(t_lista_circular*, entradaTablaPaginas*);
void insertar_lista_circular(t_lista_circular*, entradaTablaPaginas*);
t_lista_circular* obtener_lista_circular_del_proceso(int);
uint es_victima_clock(entradaTablaPaginas*);
uint es_victima_clock_modificado_um(entradaTablaPaginas*);
uint es_victima_clock_modificado_u(entradaTablaPaginas*);
void algoritmo_clock(t_lista_circular*, entradaTablaPaginas*);
void algoritmo_clock_modificado(t_lista_circular*, entradaTablaPaginas*);
void sustitucion_paginas(entradaTablaPaginas*, int);
void actualizar_registros(entradaTablaPaginas*, entradaTablaPaginas*);

#endif /* FUNCIONES_MEMORIA_H_*/

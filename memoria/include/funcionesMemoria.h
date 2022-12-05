#ifndef FUNCIONES_MEMORIA_H_
#define FUNCIONES_MEMORIA_H_

#include <sharedUtils.h>

#include <stdbool.h>
#include <errno.h>
#include <unistd.h> //Para el truncate
#include <inttypes.h>

#define IP_MEMORIA "127.0.0.1"

/* *** DECLARO STRUCTS *** */
typedef struct {
	int numeroDeEntrada;
	int numeroMarco;
	int presencia;
	int uso;
	int modificado;
	int posicionEnSwap;
	int numeroDeSegmento;
} entradaTablaPaginas;

typedef struct {// capaz usar diccionario
	t_list* entradas;
	int pid;
	int numeroDeSegmento;
} tablaDePaginas;

typedef struct {
	int numeroDeMarco;//Hacer lista de marcos, el indice en la lista es el numero de marco, la cantidad de marcos es
	int marcoLibre;// tam memoria/tam pagina
} marco;

/* *** DECLARO VARIABLES *** */
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

/* ***DECLARO FUNCIONES *** */
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
int algoritmoClock(t_list*,entradaTablaPaginas*);
int algoritmoClockM (t_list*,entradaTablaPaginas*);
marco* siguienteMarcoLibre();
marco* buscarMarco(int);
int indiceDeEntradaAReemplazar(int);
void finalizacionDeProceso(int);
int buscarNroTablaDePaginas(int);
void modificarPaginaACargar(entradaTablaPaginas*, int);
void cargarPagina(entradaTablaPaginas*);
void liberarEspacioEnMemoria(tablaDePaginas*);

void enviarTamanioDePaginaYCantidadDeEntradas(int);

//Variables de conexiones
extern int clienteCpu;

void escribirElPedido(uint32_t,int, int);
entradaTablaPaginas* entradaCargadaConMarcoAsignado(int);
uint32_t leerElPedido(int, int);

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

#endif /* FUNCIONES_MEMORIA_H_*/

#ifndef UTILS_H_
#define UTILS_H_

#include <shared/hello.h>

extern t_log* logger;
extern t_list* listaInstrucciones;
#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"

typedef enum
{
	DISPOSITIVO_E_S,
	PANTALLA,
	TECLADO
} dispositivos_IO;

typedef struct
{
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;


// En principio, no hace falta ------------------------------------
typedef struct {
	char* dispositivo;
	long tiempo;
	t_queue* cola_procesos;
	struct t_elem_disp* next;
} t_elem_disp;
// En principio, no hace falta ------------------------------------

/*
La idea seria tener las siguientes funciones:
- recibir instrucciones
- recibir tamanios de segmentos
- enviar confirmacion de recepcion
- enviar pedido de impresion
- enviar valor a imprimir
- enviar pedido de valor por teclado
- recibir mensaje de desbloqueo teclado
- recibir valor de teclado
*/

//typedef struct
//{
//	int valor;
//	char* registro;
//	char* otroRegistro;
//}parametro;


typedef struct
{
	char* identificador;
	//parametro* parametros;
	int parametros[2];
} instruccion;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

typedef struct
{
	int numeroSegmento;
	int tamanioSegmento;
	int numeroTablaPaginas;//Revisar nombre
}entradaTablaSegmento;

typedef struct
{
	int idProceso;
	t_list* instrucciones;
	int programCounter;
	t_registros registros;
	t_list* tablaSegmentos;
	//int socket;
	t_estado estado;
} t_pcb;

extern int socketServidor;

t_list* recibir_paquete_instrucciones(int);

t_list * inicializar_tabla_segmentos(int);
void inicializar_registros(int v[4]);

void enviSar_entero(int valor, int socket_cliente, int cod_op);
void recibir_consola(int);
void atender_consola(int);
void inicializar_configuraciones(char* unaConfig);
void inicializar_listas_y_colas();
void inicializar_lista_dispositivos();
void iterator(instruccion*);
int conexionConConsola();

extern char * ipMemoria;
extern char * puertoMemoria;
extern char * ipCpu;
extern char * puertoCpuDispatch;
extern char * puertoCpuInterrupt;
extern char * puertoKernel;
extern char * algoritmoPlanificacion;
extern int gradoMultiprogramacionTotal;
extern char** dispositivos_io;
extern char** tiempos_io;
extern t_list colas_dispositivos_io;
extern int quantum_rr;

extern sem_t kernelSinFinalizar;

extern int conexionCpu;

extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int contadorSegmentos;
extern int desplazamiento;

void obtenerTamanioIdentificadores(instruccion*);
void obtenerCantidadDeSegmentos(entradaTablaSegmento*);
void agregarInstruccionesAlPaquete(instruccion*);
void agregarSegmentosAlPaquete(entradaTablaSegmento*);
void agregar_a_paquete_kernel_cpu(t_pcb*,int,int);

#endif /* UTILS_H_ */

#ifndef FUNCIONES_CPU_H_
#define FUNCIONES_CPU_H_

//--------------------INCLUDES
//Copie todas las librerias del tp pasado
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/string.h>
#include<commons/config.h>
#include<string.h>
#include<assert.h>
#include<math.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>
//--------------PUERTOS
//Revisar
#define IP_CPU "127.0.0.1"

//-------------------STRUCS
typedef enum//REVISAR LOS ENUM
{
	MENSAJE_CPU_MEMORIA,
	TAM_PAGINAS_Y_CANT_ENTRADAS,
	PRIMER_ACCESO,
	SEGUNDO_ACCESO,
	READ,
	WRITE,
	COPY,
	RELLENO,
	KERNEL_PCB_A_CPU
}op_code;
//
//typedef enum//REVISAR LOS ENUM
//{
//	MENSAJE_A_KERNEL,
//	RECIBIR_PCB,
//	I_O,
//	EXIT,
//	INTERRUPT,
//	MENSAJE_INTERRUPT
//}op_code_kernel;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

extern t_log* logger;
extern t_config* config;
extern t_paquete* paquete;

extern t_list* tlb;

//--------------  Cpu como servidor de Kernel ---------
extern int clienteKernel;
extern int server_fd;
extern char* ipMemoria;
extern int puertoMemoria;

extern int puertoDeEscuchaDispatch;
extern int puertoDeEscuchaInterrupt;

extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int desplazamiento;
extern int conexionMemoria;

extern uint32_t ax;
extern uint32_t bx;
extern uint32_t cx;
extern uint32_t dx;

typedef struct
{
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;

typedef struct
{
	int numeroSegmento;
	int tamanioSegmento;
	int numeroTablaPaginas;
}entradaTablaSegmento;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

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
//Saque el IO[2] porque justamente es una instruccion que ya sabemos los valores se deberia enviar aparte.

//typedef struct
//{
//	int idProceso;
//	int tamanioProceso;
//	t_list* instrucciones;
//	int programCounter;
//	int nroTabla1erNivel; // Esto se lo pasa memoria
//	float estimacionRafaga;
//	clock_t rafagaMs; //pasar a int
//	clock_t horaDeIngresoAExe;
//	t_estado estado;
//} t_pcb;

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

typedef struct
{
	int nroDeProceso;
	int nroDeSegmento;
	int nroDePagina;
	int nroDeMarco;
	time_t instanteGuardada;
	time_t ultimaReferencia;
} entradaTLB;

typedef enum
{
	AX,
	BX,
	CX,
	DX
}registros;

typedef enum{
	SET,
	ADD,
	MOV_IN,
	MOV_OUT,
	IO,
	EXT
}identificadorInstruccion;

typedef enum
{
	DISCO = 0,
	PANTALLA = 1,
	TECLADO = 2
}dispositivos_IO;


//--------------------VARIABLES
extern t_log* logger;
extern t_config* config;

extern t_pcb* unPcb;

//----------------------------DECLARO FUNCIONES
int chequeoCantidadArchivos(int);
void inicializarConfiguraciones(char*);
t_list* recibir_paquete(int);
t_pcb* recibir_pcb(int);
void obtenerTamanioIdentificadores(instruccion*);
void agregarInstruccionesAlPaquete(instruccion*);

void obtenerCantidadDeSegmentos(entradaTablaSegmento*);
void agregarSegmentosAlPaquete(entradaTablaSegmento*);

uint32_t registroAUtilizar(int);
int chequeoDeDispositivo(char*);

instruccion* buscarInstruccionAEjecutar(t_pcb*);
void leerTamanioDePaginaYCantidadDeEntradas(t_list*);
void ejecutar(instruccion*,t_list*);
int decode(instruccion*);
int buscarDireccionFisica(int,t_list*);
int chequeoDeDispositivo(char*);
int chequearMarcoEnTLB(int);
void calculosDireccionLogica(int,t_list*);

void chequeoDeSF(int, int,t_list *);

t_list* inicializarTLB();
void reiniciarTLB();
//----------------------------FUNCIONES DE CONEXIONES
int iniciar_servidor(int);
void iterator(instruccion*);
void* serializar_paquete(t_paquete*,int);
t_paquete* crear_paquete(int);
void eliminar_paquete(t_paquete*);
int esperar_cliente(int);
int recibir_operacion(int);

int crear_conexion(char *, int);
int conexionConKernelDispatch(void);
int conexionConMemoria(void);

void enviar_mensaje(char*, int,int);

#endif /*FUNCIONES_CPU_H_*/

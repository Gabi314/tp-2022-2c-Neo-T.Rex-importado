#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/config.h>
#include<stdbool.h>
#include<assert.h>
#include<math.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>
#define IP_CPU "127.0.0.1"

typedef enum {
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_SOLICITUD_VALOR_POR_TECLADO,
	KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS,
	KERNEL_MENSAJE_DESBLOQUEO_TECLADO,
	KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO,
	KERNEL_MENSAJE_FINALIZAR_CONSOLA
}op_code;

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
	int numeroTablaPaginas;//Revisar nombre
}entradaTablaSegmento;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

typedef struct
{
	int idProceso;
	t_list* instrucciones;
	int programCounter;
	t_registros registros;
	t_list* tabla_segmentos;
	int socket;
	t_estado estado;
} t_pcb;
//Saque el IO[2] porque justamente es una instruccion que ya sabemos los valores se deberia enviar aparte.

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

void * recibir_buffer(int* size, int socket_cliente);
//----------------------------FUNCIONES DE CONEXIONES
int iniciar_servidor(int);
void iterator(instruccion*);
void* serializar_paquete(t_paquete*,int);
t_paquete* crear_paquete(int);
void eliminar_paquete(t_paquete*);
int esperar_cliente(int);
int recibir_operacion(int);

int crear_conexion(char *, char *);
int conexionConKernel(void);
int conexionConMemoria(void);

void enviar_mensaje(char*, int,int);

#endif /* UTILS_H_ */

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

//--------------PUERTOS
//Revisar
#define IP_CPU "127.0.0.1"
#define PUERTO_CPU_DISPATCH 8001 // por ahora este faltan los otros puertos para conectar a kernel
#define PUERTO_CPU_INTERRUPT 8005

//-------------------STRUCS
typedef enum//REVISAR LOS ENUM
{
	MENSAJE_CPU_MEMORIA,
	TAM_PAGINAS_Y_CANT_ENTRADAS,
	PRIMER_ACCESO,
	SEGUNDO_ACCESO,
	READ,
	WRITE,
	COPY
}op_code_memoria;

typedef enum//REVISAR LOS ENUM
{
	MENSAJE_A_KERNEL,
	RECIBIR_PCB,
	I_O,
	EXIT,
	INTERRUPT,
	MENSAJE_INTERRUPT
}op_code_kernel;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code_memoria codigo_operacion_memoria;
	op_code_kernel codigo_operacion_kernel;
	t_buffer* buffer;
} t_paquete;

extern t_log* logger;
extern t_config* config;
extern t_paquete* paquete;

//--------------  Cpu como servidor de Kernel ---------
extern int clienteKernel;
extern int server_fd;
extern char* ipMemoria;
extern int puertoMemoria;

extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int desplazamiento;
extern int conexionMemoria;

typedef struct
{
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

typedef struct
{
	int idProceso;
	t_list* instrucciones;
	int programCounter;
	t_registros registros ; // quedan como strings, al menos hasta cpu
	t_list * tabla_segmentos; // cada elemento de la lista tendria un vector de dos posiciones (una para el tamanio del
							// segmento y otra para el número o identificador de tabla de páginas asociado a cada uno)
	int socket;
	t_estado estado;
} t_pcb;


typedef struct
{
	int valor;
	char* registro;
	char* otroRegistro;
}parametro;

typedef struct
{
	char* identificador;
	//parametro* parametros;
	int parametros[2];
} instruccion;

typedef enum{
	SET,
	ADD,
	MOV_IN,
	MOV_OUT,
	IO,
	EXT
}identificadorInstruccion;


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

instruccion* buscarInstruccionAEjecutar(t_pcb*);
void leerTamanioDePaginaYCantidadDeEntradas(t_list*);

//----------------------------FUNCIONES DE CONEXIONES
int iniciar_servidor(int);
void iterator(instruccion*);
void* serializar_paquete(t_paquete*,int);
t_paquete* crear_paquete(int);
void eliminar_paquete(t_paquete*);
int esperar_cliente(int);
int recibir_operacion(int);

int conexionConKernel(void);
int conexionConMemoria(void);



#endif /*FUNCIONES_CPU_H_*/

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
#include<semaphore.h>
#include<pthread.h>
#include<stdbool.h>
#include<assert.h>

extern t_log* logger;
extern t_list* listaInstrucciones;
#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"


typedef enum {
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_DESBLOQUEAR_TECLADO,
	KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS,
	//KERNEL_MENSAJE_PEDIDO_IMPRESION_POR_PANTALLA, Me parece que no serviria mandar un mensaje, mandamos el valor a imprimir de una y listo
	KERNEL_MENSAJE_DESBLOQUEO_TECLADO,
	KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO
}op_code;

typedef enum
{
	DISCO,
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

extern t_queue* colaNew;
extern t_queue* colaReadyFIFO;
extern t_queue* colaReadyRR;
extern t_queue* colaBlockedPantalla;
extern t_queue* colaBlockedTeclado;
extern t_list* listaDeColasDispositivos;


typedef struct{
	int num_segmento;
	int tam_segmento;
	int num_tabla_paginas;
	struct t_tabla_segmentos* next;
}t_tabla_segmentos;


// En principio, no hace falta ------------------------------------
typedef struct {
	char* dispositivo;
	int tiempo;
	t_queue* cola_procesos;
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

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;
typedef enum algoritmo{FIFO,RR} t_algoritmo_pcb;

typedef struct
{
	int idProceso;
	int tamanioProceso; // se usa en memoria
	t_list* instrucciones;
	int program_counter;
	t_registros registros ;
	t_list* tabla_segmentos; // cada elemento de la lista tendria un vector de dos posiciones (una para el tamanio del
							// segmento y otra para el número o identificador de tabla de páginas asociado a cada uno)
	int socket;
	t_estado estado;
} t_pcb;


int recibir_entero(int socket_cliente);
extern int socketServidor;
t_list* recibir_lista_enteros(int socket_cliente);
t_list* recibir_paquete(int);
t_list* recibir_paquete_instrucciones(int);
//t_list* recibir_lista_instrucciones(int);
t_list * inicializar_tabla_segmentos(int);
void inicializar_registros(t_registros registros);
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente, int cod_op);
t_paquete* crear_paquete(int);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void * recibir_buffer(int* size, int socket_cliente);
int esperar_cliente(int socket_servidor);
void enviSar_entero(int valor, int socket_cliente, int cod_op);
void recibir_consola(int);
void atender_consola(int);
int get_identificador();

void inicializar_configuraciones(char* unaConfig);
void inicializar_listas_y_colas();
void inicializar_lista_dispositivos();
void inicializar_colas();
void inicializar_semaforos();

void iterator(instruccion*);

int recibir_operacion(int);
int iniciar_servidor(void);
int conexionConConsola();

void agregarANew(t_pcb* proceso);
t_pcb* sacarDeNew();

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

extern int identificadores_pcb;

extern sem_t kernelSinFinalizar;
extern sem_t gradoDeMultiprogramacion;
extern pthread_mutex_t mutexNew;


#endif /* UTILS_H_ */

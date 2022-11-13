#ifndef UTILS_H_
#define UTILS_H_

#include<signal.h>
#include<semaphore.h>
#include<pthread.h>
#include<stdbool.h>
#include<assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>


extern t_log* logger;
extern t_list* listaInstrucciones;
#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"
//#define CONFIG_FILE "../src/config/kernel.config"


typedef enum {
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_SOLICITUD_VALOR_POR_TECLADO,
	KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS,
	//KERNEL_MENSAJE_PEDIDO_IMPRESION_POR_PANTALLA, Me parece que no serviria mandar un mensaje, mandamos el valor a imprimir de una y listo

	KERNEL_MENSAJE_DESBLOQUEO_TECLADO,
	KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO,
	KERNEL_MENSAJE_FINALIZAR_CONSOLA,
	TERMINAR_PROCESO,	// cpu manda proceso para terminarlo
	IO_GENERAL,		// cpu manda proceso por io generica
	IO_TECLADO,		// cpu manda proceso por io teclado
	IO_PANTALLA,		// cpu manda proceso por io pantalla
	RECIBIR_ENTERO,
	QUANTUM,
	DESALOJAR_PROCESO_POR_FIN_DE_QUANTUM,
	PAGE_FAULT
}op_code;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

typedef enum algoritmo { FIFO, RR } t_algoritmo_pcb;

typedef enum
{
	DISCO,
	PANTALLA,
	TECLADO
} dispositivos_IO;

typedef struct {
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;

typedef struct {
	int idProceso;
	t_list* instrucciones;
	int program_counter;
	t_registros registros ;
	t_list* tabla_segmentos; // cada elemento de la lista tendria un vector de dos posiciones (una para el tamanio del
	// segmento y otra para el número o identificador de tabla de páginas asociado a cada uno)
	int socket;
	t_estado estado;
	t_algoritmo_pcb algoritmoActual;
} t_pcb;

typedef struct {
	char* identificador;
	//parametro* parametros;
	int parametros[2];
} instruccion;

typedef struct {
	int numeroSegmento;
	int tamanioSegmento;
	int numeroTablaPaginas;
}entradaTablaSegmento;

extern t_queue* colaNew;
extern t_queue* colaReadyFIFO;
extern t_queue* colaReadyRR;
//extern t_queue* colaBlockedPantalla; No hacen falta, cada consola tiene su pantalla y teclado
//extern t_queue* colaBlockedTeclado;
extern t_list* listaDeColasDispositivos;


typedef struct{
	int tam_segmento;
	int num_tabla_paginas;
}t_tabla_segmentos;


// En principio, no hace falta ------------------------------------
typedef struct {
	char* dispositivo;
	int tiempo;
	t_queue* cola_procesos;
	t_queue* cola_UTs;
	sem_t semaforo;

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
	t_pcb* pcb;
	int registro;
} t_info_teclado;

typedef struct {
	t_pcb* pcb;
	int registro;
} t_info_pantalla;


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
void enviar_entero(int valor, int socket_cliente, int cod_op);
char * recibir_cadena(int socket_cliente);


int get_identificador();
t_pcb* recibir_pcb(int socket_cliente);

void inicializar_configuraciones(char* unaConfig);
void inicializar_listas_y_colas();
void inicializar_lista_dispositivos();
void inicializar_colas();
void inicializar_semaforos();
void levantar_hilo_dispositivo(t_elem_disp*);

void iterator(instruccion*);

int recibir_operacion(int);
int iniciar_servidor(void);
t_pcb * conexionConConsola();
int conexionConCpu(t_pcb * PCB);

void agregarANew(t_pcb* proceso);
t_pcb* sacarDeNew();

t_pcb* obtenerSiguienteDeReady();
t_pcb* obtenerSiguienteFIFO();
t_pcb* obtenerSiguienteRR();
void ejecutar(t_pcb* proceso);

void recibir_consola(int * servidor) ;
void atender_consola(int * nuevo_cliente);
void asignar_memoria();
void readyAExe();
void atender_interrupcion_de_ejecucion();
void atender_IO_teclado(t_info_teclado * info);
void atender_IO_pantalla(t_info_pantalla * info);
void atender_IO_generico(t_elem_disp*);
void controlar_quantum();
void atender_page_fault(t_pcb* pcb);

void terminarEjecucion(t_pcb*);
void destruirProceso(t_pcb*);
int buscar_valor_registro(t_pcb*,int);
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
extern t_paquete* paquete;

extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int contadorSegmentos;
extern int desplazamiento;


extern t_pcb* pcbTeclado;
extern int registroTeclado;
extern t_pcb* pcbPantalla;
extern int registroPantalla;

extern int identificadores_pcb;

extern pthread_t hiloQuantumRR;

extern sem_t kernelSinFinalizar;
extern sem_t gradoDeMultiprogramacion;
extern sem_t cpuDisponible;
extern sem_t pcbEnNew;
extern sem_t pcbEnReady;
extern pthread_mutex_t mutexNew;
extern pthread_mutex_t obtenerProceso;
extern pthread_mutex_t mutexPantalla;
extern pthread_mutex_t mutexTeclado;



#endif /* UTILS_H_ */

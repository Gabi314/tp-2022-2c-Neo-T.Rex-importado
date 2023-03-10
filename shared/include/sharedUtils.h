#ifndef SHARED_HELLO_H_
#define SHARED_HELLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>
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
#include<math.h>
#include<time.h>
#include <arpa/inet.h>
#include <stdint.h>

typedef enum {
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_MENSAJE_DISPOSITIVOS_IO,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_DESBLOQUEAR_TECLADO,
	KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO,
	KERNEL_MENSAJE_CONFIRMACION_RECEPCION_INSTRUCCIONES_SEGMENTOS,
	KERNEL_MENSAJE_FINALIZAR_CONSOLA,
	//KERNEL_MENSAJE_PEDIDO_IMPRESION_POR_PANTALLA, Me parece que no serviria mandar un mensaje, mandamos el valor a imprimir de una y listo
	KERNEL_MENSAJE_DESBLOQUEO_TECLADO,
	KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO,
	KERNEL_PCB_A_CPU,
	IO_GENERAL,
	IO_TECLADO,
	IO_PANTALLA,
	QUANTUM,
	DESALOJAR_PROCESO_POR_FIN_DE_QUANTUM,
	KERNEL_MENSAJE_SOLICITUD_CARGAR_PAGINA,
	KERNEL_MENSAJE_CONFIRMACION_PF,
	KERNEL_A_MEMORIA_PAGINA_A_CARGAR,
	KERNEL_A_MEMORIA_MENSAJE_LIBERAR_POR_TERMINADO,
	KERNEL_A_MEMORIA_PID_PARA_FINALIZAR,
	KERNEL_A_MEMORIA_PAGE_FAULT,
	MENSAJE_CPU_MEMORIA,
	TAM_PAGINAS_Y_CANT_ENTRADAS,
	PRIMER_ACCESO,
	OBTENER_MARCO,
	CPU_DISPOSITIVO_A_KERNEL,
	//KERNEL_MENSAJE_INTERRUPT,
	CPU_A_KERNEL_INGRESAR_VALOR_POR_TECLADO,
	CPU_A_KERNEL_MOSTRAR_REGISTRO_POR_PANTALLA,
	CPU_PCB_A_KERNEL_POR_IO,
	CPU_PCB_A_KERNEL_POR_IO_TECLADO,
	CPU_PCB_A_KERNEL_POR_IO_PANTALLA,
	CPU_A_KERNEL_PCB_POR_DESALOJO,
	CPU_PCB_A_KERNEL_PCB_POR_FINALIZACION,
	CPU_A_KERNEL_UNIDADES_DE_TRABAJO_IO,
	CPU_A_KERNEL_PCB_PAGE_FAULT,
	CPU_A_KERNEL_PAGINA_PF,
	CPU_A_MEMORIA_LEER,
	CPU_A_MEMORIA_VALOR_A_ESCRIBIR,

	MEMORIA_A_CPU_NUMERO_MARCO,
	MEMORIA_A_CPU_NUMERO_LEIDO,
	MEMORIA_A_KERNEL_NUMERO_TABLA_PAGINAS,
	MEMORIA_A_CPU_PAGE_FAULT,

	NRO_TP
} op_code;


typedef struct {
	int size;
	void *stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

typedef struct
{
	char* identificador;
	int parametros[2];
} instruccion;


typedef struct
{
	int numeroSegmento;//revisar   [(0,64,n),(1,128,n+1),..]
	int tamanioSegmento;
	int numeroTablaPaginas;
}entradaTablaSegmento;

typedef struct
{
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;
typedef enum algoritmo { FIFO, RR } t_algoritmo_pcb;

typedef struct
{
	int idProceso;
	t_list* instrucciones;
	int programCounter;
	t_registros registros;
	t_list* tablaSegmentos;
	int socket;
	t_estado estado;
	t_algoritmo_pcb algoritmoActual;
} t_pcb;

typedef struct {
    t_log* log;
    int fd;
    char* server_name;
} t_procesar_conexion_args;

int say_hello(char *who);

t_log* iniciar_logger(char*, char*);

void enviar_entero(int valor, int socket_cliente, int cod_op);

t_list* recibir_lista_enteros(int socket_cliente);

int recibir_entero(int socket_cliente);

int recibir_operacion(int socket_cliente);

void* recibir_buffer(int *size, int socket_cliente);

char* recibir_mensaje(int socket_cliente);

void* serializar_paquete(t_paquete *paquete, int bytes);

int iniciar_servidor(char *ip, char *puerto, char *cliente);

int esperar_cliente(int socket_servidor, char *cliente);

int crear_conexion(char *ip, char *puerto);

void enviar_mensaje(char *mensaje, int socket_cliente, int cod_op);

void crear_buffer(t_paquete *paquete);

t_paquete* crear_paquete(int cod_op);

void agregar_a_paquete_unInt(t_paquete *paquete, void *valor, int tamanio);

void enviar_paquete(t_paquete *paquete, int socket_cliente);

void eliminar_paquete(t_paquete*);

void liberar_conexion(int);

char** recibirListaDispositivos(int);

//t_pcb* recibir_pcb(int);

void iteratorMostrarInstrucciones(instruccion* instruccion);

void controlar_pcb(t_pcb*);

#endif

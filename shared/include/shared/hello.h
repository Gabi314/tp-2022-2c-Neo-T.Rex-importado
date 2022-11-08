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

typedef enum {
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_MENSAJE_DISPOSITIVOS_IO,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_DESBLOQUEAR_TECLADO,
	KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO,
	KERNEL_MENSAJE_FINALIZAR_CONSOLA,
	//KERNEL_MENSAJE_PEDIDO_IMPRESION_POR_PANTALLA, Me parece que no serviria mandar un mensaje, mandamos el valor a imprimir de una y listo
	KERNEL_MENSAJE_DESBLOQUEO_TECLADO,
	KERNEL_PAQUETE_VALOR_RECIBIDO_DE_TECLADO,
	KERNEL_PCB_A_CPU,
	MENSAJE_CPU_MEMORIA,
	TAM_PAGINAS_Y_CANT_ENTRADAS,
	PRIMER_ACCESO,
	SEGUNDO_ACCESO,
	CPU_PCB_A_KERNEL,
	CPU_DISPOSITIVO_A_KERNEL
} op_code;

typedef struct {
	int size;
	void *stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;


int say_hello(char *who);

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

#endif

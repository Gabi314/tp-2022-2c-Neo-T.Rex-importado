#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<readline/readline.h>

#include <sys/stat.h>
#include <fcntl.h>

typedef enum
{
	KERNEL_PAQUETE_TAMANIOS_SEGMENTOS,
	KERNEL_PAQUETE_INSTRUCCIONES,
	KERNEL_PAQUETE_VALOR_A_IMPRIMIR,
	KERNEL_MENSAJE_VALOR_IMPRESO,
	KERNEL_MENSAJE_PEDIDO_VALOR_POR_TECLADO,
	KERNEL_MENSAJE_FINALIZAR_CONSOLA,
	PRUEBA
}op_code;

typedef enum
{
	DISCO = 0,
	PANTALLA = 1,
	TECLADO = 2
}dispositivos_IO;

typedef enum
{
	AX,
	BX,
	CX,
	DX
}registros;

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

//Estructura auxiliar de parametros, por las dudas
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

//------------------ DECLARACION
extern t_log* logger;
extern t_config* config;
extern char* ipKernel;
extern char* puertoKernel;
extern char** segmentos;
extern int tiempoPantalla;
extern int conexion;

//------------------ DECLARACION FUNCIONES DE CONEXIONES
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char*,int,int);
t_paquete* crear_paquete(int);
void agregar_a_paquete_instrucciones(t_paquete*, instruccion*, int);
void agregar_a_paquete_segmentos(t_paquete*,void*,int);
void enviarPaqueteTamanioDeSegmentos();
void enviarListaInstrucciones(t_paquete*);

void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void liberar_conexion(int socket_cliente);

void agregarAPaqueteSegmentos(char**,t_paquete*);
void dividirInstruccionesAlPaquete(t_log*,t_paquete*,char**,instruccion*);

int chequeoCantidadArchivos(int);
void inicializarConfiguraciones(char*);

int recibir_operacion(int);
void recibir_mensaje(int);
t_list* recibir_paquete(int);

//------------------ DECLARACION FUNCIONES
FILE* abrirArchivo(char*);
FILE* recorrerArchivo(char*,FILE*);

void imprimirValorPorPantalla(int);
void solicitudIngresarValorPorTeclado(int);

int chequeoDeRegistro(char*);
int chequeoDeDispositivo(char*);
void atenderPeticionesKernel();


#endif /* UTILS_H_ */

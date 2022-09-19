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
	MENSAJE,
	INSTRUCCIONES
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

typedef struct
{
	int valor;
	char* registro;
	char* otroRegistro;
}parametro;


typedef struct
{
	char* identificador;
	parametro* parametros;
} instruccion;


extern t_log* logger;
extern t_config* config;
extern char* ipKernel;
extern char* puertoKernel;
extern char** segmentos;
extern int conexion;

void inicializarConfiguraciones();
void dividirInstruccionesAlPaquete(t_log*,t_paquete*,char**,instruccion*);

int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char*,int,int);
t_paquete* crear_paquete(int);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

#endif /* UTILS_H_ */

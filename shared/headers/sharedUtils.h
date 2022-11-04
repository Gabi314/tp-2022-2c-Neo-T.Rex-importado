/*
 * sharedUtils.h
 *
 *  Created on: Oct 30, 2022
 *      Author: utnso
 */

#ifndef HEADERS_SHAREDUTILS_H_
#define HEADERS_SHAREDUTILS_H_

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

// Firmas

t_config* inicializar_configuracion(char*);

int esperar_cliente(int);

// Enums

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;

typedef enum algoritmo { FIFO, RR } t_algoritmo_pcb;

// Estructuras

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


#endif /* HEADERS_SHAREDUTILS_H_ */

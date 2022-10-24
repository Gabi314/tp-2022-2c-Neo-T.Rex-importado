#include <stdio.h>
#include <stdlib.h>


#include "funcionesCpu.h"

int main(void) {
	logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);

	log_info(logger,"iniciamos servidor con un puerto, esperando a kernel");
	int servidor_fd = iniciar_servidor(8001);
	int puertoInterrupt = esperar_cliente(servidor_fd);
	log_info(logger,"se conecto kernel al primer puerto");

	log_info(logger,"iniciamos servidor con otro puerto, esperando a kernel");
	int segundoServidor_fd = iniciar_servidor(8002);
	int puertoDispatch = esperar_cliente(segundoServidor_fd);

	log_info(logger,"se conecto kernel al segundo puerto");
/*
	recibir_operacion(puertoInterrupt);
	recibir_entero(puertoInterrupt);
	enviar_entero(20, puertoInterrupt, 0);
*/
	recibir_operacion(puertoDispatch);
	recibir_entero(puertoDispatch);
	enviar_entero(40, puertoDispatch, 0);

	return EXIT_SUCCESS;
}

/*
 * sharedUtils.c
 *
 *  Created on: Oct 30, 2022
 *      Author: utnso
 */

#include "headers/sharedUtils.h"

t_config* inicializar_configuracion(char* file){
	t_config* config = config_create(file);
	if(config  == NULL){
		perror("No se pudo leer la configuracion: ");
		printf("Error leyendo archivo de configuración. \n");
		exit(-1);
	}
	return config;
}

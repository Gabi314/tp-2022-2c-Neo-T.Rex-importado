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

t_config* inicializar_configuracion(char*);


#endif /* HEADERS_SHAREDUTILS_H_ */

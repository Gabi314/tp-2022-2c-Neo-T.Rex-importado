#include "funcionesMemoria.h"

int flagDeEntradasPorTabla;
int numeroDeMarco;
int desplazamiento;
int pidActual;

int conexionConCpu(void* void_args) {
	static pthread_mutex_t mutexMemoriaData;
	// int server_fd = iniciar_servidor(IP_MEMORIA,puertoMemoria,"Cpu"); // tendria que estar comentado porque viene despues de coenxion con kernel
	// clienteCpu = esperar_cliente(server_fd,"Cpu");

	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;

	int clienteCpu = args->fd;

	log_info(loggerAux, "Memoria lista para recibir a Cpu");

	t_list* listaQueContieneNroTabla1erNivelYentrada = list_create();
	t_list* listaConTablaDePaginaYPagina = list_create();
	int numeroTablaDePaginas;
	int numeroDeLaPagina;
	t_list* listaQueContieneDireccionFisca = list_create();
	int direccionFisica;
	t_list* listaQueContieneValorAEscribir = list_create();
	t_list* listaQueContieneDirFisica1YDirFisica2 = list_create();

	while(1) {
//		pthread_mutex_lock(&mutexMemoriaData);
		int cod_op = recibir_operacion(clienteCpu);

		switch (cod_op) {

			case PRIMER_ACCESO://ES OBTENER MARCO
				listaConTablaDePaginaYPagina = recibir_lista_enteros(clienteCpu);

				numeroTablaDePaginas = list_get(listaConTablaDePaginaYPagina, 0);
				numeroDeLaPagina = list_get(listaConTablaDePaginaYPagina, 1);

				numeroDeMarco =  marcoSegunIndice(numeroTablaDePaginas, numeroDeLaPagina);

				if(numeroDeMarco == -1) {
					enviar_mensaje("La pagina no esta en memoria", clienteCpu, MEMORIA_A_CPU_PAGE_FAULT);
				} else {
					log_info(logger, "PID: <%d> - Página: <%d> - Marco: <%d>", pidActual, numeroDeLaPagina, numeroDeMarco);
					enviar_entero(numeroDeMarco,clienteCpu,MEMORIA_A_CPU_NUMERO_MARCO);
				}

				break;
			case CPU_A_MEMORIA_LEER: //ESTE VA A SER EL READ
				listaQueContieneDireccionFisca = recibir_lista_enteros(clienteCpu);

				numeroDeMarco = (int) list_get(listaQueContieneDireccionFisca, 0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				desplazamiento = (int) list_get(listaQueContieneDireccionFisca, 1);
				int pidMovIn = (int) list_get(listaQueContieneDireccionFisca, 2);


				log_info(loggerAux, "Me llego el marco %d con desplazamiento %d", numeroDeMarco, desplazamiento);
				log_info(loggerAux, "-------------------MOV_IN-------------------");

				uint32_t numeroALeer = leerElPedido(numeroDeMarco, desplazamiento,pidMovIn);
				log_info(loggerAux, "Envio a cpu el valor leido: %u", numeroALeer);

				enviar_entero(numeroALeer, clienteCpu, MEMORIA_A_CPU_NUMERO_LEIDO);

				log_info(loggerAux, "-------------------MOV_IN-------------------\n");

				break;
			case CPU_A_MEMORIA_VALOR_A_ESCRIBIR://caso: me envia dir fisica y escribo el valor en esa direccion
				listaQueContieneDireccionFisca = recibir_lista_enteros(clienteCpu);

				numeroDeMarco = (int) list_get(listaQueContieneDireccionFisca, 0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				desplazamiento = (int) list_get(listaQueContieneDireccionFisca, 1);

				uint32_t valorAEscribir = (uint32_t) list_get(listaQueContieneDireccionFisca, 2);
				int pidMovOut = (int) list_get(listaQueContieneDireccionFisca, 3);

				log_info(loggerAux, "-------------------MOV_OUT-------------------");
				log_info(loggerAux, "Me llego el valor a escribir: %u del proceso %d",valorAEscribir,pidMovOut);

				escribirElPedido((uint32_t) valorAEscribir, numeroDeMarco, desplazamiento, pidMovOut);

				log_info(loggerAux,"-------------------MOV_OUT-------------------\n");

				enviar_mensaje("Se escribio correctamente el valor", clienteCpu, MENSAJE_CPU_MEMORIA);

				break;
			case -1:
				log_error(loggerAux, "Se desconecto el cliente. Terminando conexion");

				return EXIT_FAILURE;
			default:
				log_warning(loggerAux, "Operacion desconocida. No quieras meter la pata");

				break;
		}
		//pthread_mutex_unlock(&mutexMemoriaData);
	}
	return EXIT_SUCCESS;
}

int conexionConKernel(void* void_args) {
	//int pidActual;

	// int socket_kernel = iniciar_servidor(IP_MEMORIA,puertoMemoria,"Kernel");
	log_info(loggerAux, "Memoria lista para recibir a Kernel");
	// clienteKernel = esperar_cliente(socket_kernel,"Kernel");

	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;

	int clienteKernel = args->fd;

	int numeroTablaDePaginas;
	int numeroPagina;
	t_list* listaQueContienePidYCantidadSegmentos = list_create();
	t_list* listaQueContieneNumTablaYPagina = list_create();

	while(1) {
		int cod_op = recibir_operacion(clienteKernel);

		switch (cod_op) {
			case NRO_TP:
				listaQueContienePidYCantidadSegmentos = recibir_lista_enteros(clienteKernel);
				pidActual = (int) list_get(listaQueContienePidYCantidadSegmentos,0);
				cantidadDeSegmentos = (int) list_get(listaQueContienePidYCantidadSegmentos, 1);

				list_create_circular(pidActual);
				enviar_entero(contNroTablaDePaginas, clienteKernel, MEMORIA_A_KERNEL_NUMERO_TABLA_PAGINAS);
				inicializarEstructuras(pidActual);//inicializo estructuras

				break;
			case KERNEL_MENSAJE_SOLICITUD_CARGAR_PAGINA:
				recibir_mensaje(clienteKernel);

				break;
			case KERNEL_A_MEMORIA_PAGE_FAULT:
				listaQueContieneNumTablaYPagina = recibir_lista_enteros(clienteKernel);
				numeroTablaDePaginas = (int) list_get(listaQueContieneNumTablaYPagina, 0);
				numeroPagina = (int) list_get(listaQueContieneNumTablaYPagina, 1);

				if(numeroPagina < entradasPorTabla) {
					tablaDePaginas* unaTablaDePaginas; //= malloc(sizeof(tablaDePaginas));
					entradaTablaPaginas* unaEntrada; // = malloc(sizeof(entradaTablaPaginas));

					unaTablaDePaginas = list_get(listaTablaDePaginas, numeroTablaDePaginas);
					unaEntrada = list_get(unaTablaDePaginas->entradas, numeroPagina);

					cargarPagina(unaEntrada,unaTablaDePaginas->pid);

					log_info(logger, "PID: <%d> - Página: <%d> - Marco: <%d>", pidActual, unaEntrada->numeroDeEntrada, unaEntrada->numeroMarco);

					enviar_mensaje("Se ha cargado la pagina correctamente", clienteKernel, KERNEL_MENSAJE_CONFIRMACION_PF);

				} else {
					log_warning(loggerAux, "El numero de pagina es mayor a la cantidad de entradas");
				}

				break;
			case KERNEL_A_MEMORIA_MENSAJE_LIBERAR_POR_TERMINADO:
				recibir_mensaje(clienteKernel);

				break;
			case KERNEL_A_MEMORIA_PID_PARA_FINALIZAR:
				int numeroDePid = recibir_entero(clienteKernel);
				finalizacionDeProceso(numeroDePid);

				break;
			case -1:
				log_error(loggerAux, "Se desconecto el cliente. Terminando conexion");

				return EXIT_FAILURE;
			default:
				log_warning(loggerAux, "Operacion desconocida. No quieras meter la pata");

				break;
			}
	}

	return EXIT_SUCCESS;
}

void enviarTamanioDePaginaYCantidadDeEntradas(int socket_cliente) {
	t_paquete* paquete = crear_paquete(TAM_PAGINAS_Y_CANT_ENTRADAS);
	log_info(loggerAux, "Envio el tamanio de pag %d y cant entradas %d",tamanioDePagina,entradasPorTabla);

	agregar_a_paquete_unInt(paquete, &tamanioDePagina, sizeof(tamanioDePagina));
	agregar_a_paquete_unInt(paquete, &entradasPorTabla, sizeof(entradasPorTabla));

	enviar_paquete(paquete, socket_cliente);
	eliminar_paquete(paquete);
}

int marcoSegunIndice(int numeroTablaDePaginas, int numeroDePagina) {
	tablaDePaginas* unaTablaDePaginas; //= malloc(sizeof(tablaDePaginas));
	entradaTablaPaginas* unaEntradaTablaDePaginas; //= malloc(sizeof(entradaTablaPaginas));

	chequeoDeIndice(numeroDePagina);
	if(flagDeEntradasPorTabla == 1) {

		unaTablaDePaginas = list_get(listaTablaDePaginas, numeroTablaDePaginas);
		unaEntradaTablaDePaginas = list_get(unaTablaDePaginas->entradas, numeroDePagina);

		flagDeEntradasPorTabla = 0;

		if(unaEntradaTablaDePaginas->presencia == 1) {
			return unaEntradaTablaDePaginas->numeroMarco;
		}
	}

	return -1;
}

void chequeoDeIndice(int indice) {
	if(indice < entradasPorTabla) {
		flagDeEntradasPorTabla = 1;
	}
}

int server_escuchar(t_log* logger, char* server_nombre, char* cliente_nombre, int server_socket) {
	static pthread_mutex_t mutexPrimerHandshake;
	// Se conecta un cliente
	log_info(logger, "Esperando a un cliente (Kernel o CPU)");
	pthread_mutex_lock(&mutexPrimerHandshake);
    int cliente_socket = esperar_cliente(server_socket, cliente_nombre);
    pthread_mutex_unlock(&mutexPrimerHandshake);
    //int cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
    	 pthread_t hilo_general;
    	// Creo un hilo para atender al cliente conectado
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_nombre;
        if (!strcmp(cliente_nombre, "KERNEL")) {

        	pthread_create(&hilo_general, NULL, (void*) conexionConKernel, (void*) args);

        } else if (!strcmp(cliente_nombre, "CPU")) {

        	int cod_op = recibir_operacion(cliente_socket);

        	if(cod_op == MENSAJE_CPU_MEMORIA){//mensaje de pedido tam pag y cant entradas
        		recibir_mensaje(cliente_socket);//recibe el pedido de tam_pag y cant_entradas
        		enviarTamanioDePaginaYCantidadDeEntradas(cliente_socket);
        	}

			pthread_create(&hilo_general, NULL, (void*) conexionConCpu, (void*) args);
		}
        pthread_detach(hilo_general);
        return 1;
    } else {
    	log_info(logger,"Socket incorrecto");
    }
    return 0;
}


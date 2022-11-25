#include "funcionesMemoria.h"

int flagDeEntradasPorTabla;
int marco;
int desplazamiento;
int clienteCpu;
int clienteKernel;

int conexionConCpu(void){
	static pthread_mutex_t mutexMemoriaData;
	int server_fd = iniciar_servidor(IP_MEMORIA,PUERTO_MEMORIA,"Cpu"); // tendria que estar comentado porque viene despues de coenxion con kernel

	log_info(logger, "Memoria lista para recibir a Cpu");
	clienteCpu = esperar_cliente(server_fd,"Cpu");

	t_list* listaQueContieneNroTabla1erNivelYentrada = list_create();
	t_list* listaConTablaDePaginaYPagina = list_create();
	int numeroTablaDePaginas;
	int numeroDeLaPagina;
	t_list* listaQueContieneDireccionFisca = list_create();
	int direccionFisica;
	t_list* listaQueContieneValorAEscribir = list_create();
	t_list* listaQueContieneDirFisica1YDirFisica2 = list_create();


	//int a = 1;
	while(1) {
		pthread_mutex_lock(&mutexMemoriaData);
		int cod_op = recibir_operacion(clienteCpu);
		sleep(retardoMemoria/1000); //lo que se tarda en acceder a memoria
		//log_info(logger,"Accediendo a memoria espere %d segundos\n",retardoMemoria/1000);
		switch (cod_op) {
			case MENSAJE_CPU_MEMORIA://mensaje de pedido tam pag y cant entradas
				recibir_mensaje(clienteCpu);//recibe el pedido de tam_pag y cant_entradas
				enviarTamanioDePaginaYCantidadDeEntradas(clienteCpu);
				break;
//			case PRIMER_ACCESO://envio de nro tabla 2do nivel
//				listaQueContieneNroTabla1erNivelYentrada = recibir_paquete_int(clienteCpu); // lista con valores de nro y entrada de tabla
//
//				nroTabla2doNivel = leerYRetornarNroTabla2doNivel(listaQueContieneNroTabla1erNivelYentrada);
//
//				enviarNroTabla2doNivel(clienteCpu,nroTabla2doNivel);
//				break;

				//CASE DEL ACCESO
			case PRIMER_ACCESO://ES OBTENER MARCO
//				numeroTablaDePaginas = recibir_entero(clienteCpu);
//				numeroDeLaPagina = recibir_entero(clienteCpu);

				listaConTablaDePaginaYPagina = recibir_paquete_int(clienteCpu);
				numeroTablaDePaginas = list_get(listaConTablaDePaginaYPagina,0);
				numeroDeLaPagina = list_get(listaConTablaDePaginaYPagina,1);

				//log_info(logger,"Me llego  la entrada de segundo nivel %d",entradaTabla2doNivel);

				marco =  marcoSegunIndice(numeroTablaDePaginas,numeroDeLaPagina);
				enviar_entero(marco,clienteCpu,MEMORIA_A_CPU_NUMERO_MARCO);

				break;
				//READ
			case SEGUNDO_ACCESO: //ESTE VA A SER EL READ
				listaQueContieneDireccionFisca = recibir_lista_enteros(clienteCpu);

				marco = (int) list_get(listaQueContieneDireccionFisca,0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				desplazamiento = (int) list_get(listaQueContieneDireccionFisca,1);
				int mov_in = (int) list_get(listaQueContieneDireccionFisca,2);

				log_info(logger,"Me llego el marco %d con desplazamiento %d",marco,desplazamiento);

				if(mov_in == 1){
					log_info(logger,"-------------------MOV_IN-------------------");

					uint32_t numeroALeer = leerElPedido(marco,desplazamiento);
					enviar_entero(numeroALeer,clienteCpu,MEMORIA_A_CPU_NUMERO_LEIDO);
					log_info(logger, "Valor leido: %u",numeroALeer);

					log_info(logger,"-------------------MOV_IN-------------------\n");
					mov_in = 0;
				}

				break;
				//WRITE
			case MOV_OUT://caso: me envia dir fisica y escribo el valor en esa direccion
				listaQueContieneDireccionFisca = recibir_lista_enteros(clienteCpu);
				marco = (int) list_get(listaQueContieneDireccionFisca,0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				desplazamiento = (int) list_get(listaQueContieneDireccionFisca,1);
				uint32_t valorAEscribir = (uint32_t) list_get(listaQueContieneValorAEscribir,2);

				log_info(logger,"-------------------MOV_OUT-------------------");

				log_info(logger,"Me llego el valor a escribir: %u",valorAEscribir);
				escribirElPedido((uint32_t) valorAEscribir,marco,desplazamiento); //casteo para que no joda el warning

				log_info(logger,"-------------------MOV_OUT-------------------\n");
				break;
//			case COPY://caso copiar
//				listaQueContieneDirFisica1YDirFisica2 = recibir_paquete_int(clienteCpu);
//
//				int marcoDeDestino = (int) list_get(listaQueContieneDirFisica1YDirFisica2,0);
//				int desplazamientoDestino = (int) list_get(listaQueContieneDirFisica1YDirFisica2,1);
//				int marcoDeOrigen = (int) list_get(listaQueContieneDirFisica1YDirFisica2,2);
//				int desplazamientoOrigen = (int) list_get(listaQueContieneDirFisica1YDirFisica2,3);
//
//				log_info(logger,"-------------------COPIAR-------------------");
//
//				copiar(marcoDeDestino,desplazamientoDestino,marcoDeOrigen,desplazamientoOrigen);
//				uint32_t datoALeer = leerElPedido(marcoDeDestino,desplazamientoDestino);
//				log_info(logger,"Se copio el valor %u en la dir fisica:(marco %d offset %d)",datoALeer,marcoDeDestino,desplazamientoDestino);
//
//				log_info(logger,"-------------------COPIAR-------------------\n");
//				break;
//			case SUSPENSION:
//
//				recibir_mensaje(clienteCpu);
//				suspensionDeProceso(pid);
//
//				break;
//			case DESUSPENSION:
//
//				recibir_mensaje(clienteCpu);
//				break;
			case -1:
				log_error(logger, "Se desconecto el cliente. Terminando conexion");
				return EXIT_SUCCESS;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}

		 pthread_mutex_unlock(&mutexMemoriaData);
	}
	return EXIT_SUCCESS;
}

int conexionConKernel(void){
	int pidActual;
	int server_fd = iniciar_servidor(); //ACA AGREGUE EL INT
	log_info(logger, "Memoria lista para recibir a Kernel");
	clienteKernel = esperar_cliente(server_fd);

	int numeroTablaDePaginas;
	int numeroPagina;
	t_list* listaQueContienePidYCantidadSegmentos = list_create();
	t_list* listaQueContieneNumTablaYPagina = list_create();
	while(1) {
		int cod_op = recibir_operacion(clienteKernel);

		switch (cod_op) {

			case NRO_TP:
				listaQueContienePidYCantidadSegmentos = recibir_paquete_int(clienteKernel);
				pidActual = (int) list_get(listaQueContienePidYCantidadSegmentos,0);
				cantidadDeSegmentos = (int) list_get(listaQueContienePidYCantidadSegmentos,1);

				enviar_entero(contNroTablaDePaginas, clienteKernel, MEMORIA_A_KERNEL_NUMERO_TABLA_PAGINAS);
				inicializarEstructuras(pidActual);//inicializo estructuras
				inicializarMarcos();

				//int nroTablaPaginas = buscarNroTablaDe1erNivel(pidActual);
				//enviarNroTabla1erNivel(clienteKernel,nroTabla1erNivel);
				return EXIT_SUCCESS;
				break;
			case PAGE_FAULT://caso: me envia dir fisica y escribo el valor en esa direccion
				listaQueContieneNumTablaYPagina = recibir_paquete_int(clienteKernel);
				numeroTablaDePaginas = (int) list_get(listaQueContieneNumTablaYPagina,0);
				numeroPagina = (int) list_get(listaQueContieneNumTablaYPagina,1);

				tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
				entradaTablaPaginas* unaEntrada = malloc(sizeof(entradaTablaPaginas));
				unaTablaDePaginas = list_get(listaTablaDePaginas,numeroTablaDePaginas);
				unaEntrada = list_get(unaTablaDePaginas->entradas,numeroPagina);
				cargarPagina(unaEntrada);
				enviar_mensaje("Se ha cargado la pagina correctamente", clienteKernel, KERNEL_MENSAJE_CONFIRMACION_PF);
				break;
			case -1:
				log_error(logger, "Se desconecto el cliente. Terminando conexion");
				return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}

	}

	return EXIT_SUCCESS;
}

void enviarTamanioDePaginaYCantidadDeEntradas(int socket_cliente){
	t_paquete* paquete = crear_paquete(TAM_PAGINAS_Y_CANT_ENTRADAS);
	log_info(logger,"Envio el tamanio de pag y cant entradas");

	agregar_a_paquete_unInt(paquete,&tamanioDePagina,sizeof(tamanioDePagina));
	agregar_a_paquete_unInt(paquete,&entradasPorTabla,sizeof(entradasPorTabla));


	enviar_paquete(paquete,socket_cliente);
	eliminar_paquete(paquete);
}

int marcoSegunIndice(int numeroTablaDePaginas,int numeroDePagina){
	tablaDePaginas* unaTablaDePaginas = malloc(sizeof(tablaDePaginas));
	entradaTablaPaginas* unaEntradaTablaDePaginas = malloc(sizeof(entradaTablaPaginas));

	chequeoDeIndice(numeroDePagina);

	if(flagDeEntradasPorTabla == 1){
		//int posicionDeLaTablaBuscada = buscarNroTablaDe2doNivel(numeroTabla2doNivel);
		unaTablaDePaginas = list_get(listaTablaDePaginas,numeroTablaDePaginas);

		unaEntradaTablaDePaginas = list_get(unaTablaDePaginas->entradas,numeroDePagina);// ya con esto puedo recuperar el marco

		flagDeEntradasPorTabla = 0;

		if(unaEntradaTablaDePaginas->presencia == 1){
			return unaEntradaTablaDePaginas->numeroMarco;
		}else{
			cargarPagina(unaEntradaTablaDePaginas);
			return unaEntradaTablaDePaginas->numeroMarco;
	//ver tema de si es una pagina con info para swap
		}
	}

}

void chequeoDeIndice(int indice){
	if(indice<entradasPorTabla){
		flagDeEntradasPorTabla = 1;
	}
}


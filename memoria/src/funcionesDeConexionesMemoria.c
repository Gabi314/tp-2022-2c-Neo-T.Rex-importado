#include "funcionesMemoria.h"

int conexionConCpu(void){

	int server_fd = iniciar_servidor(IP_MEMORIA,PUERTO_MEMORIA,"Cpu"); // tendria que estar comentado porque viene despues de coenxion con kernel

	log_info(logger, "Memoria lista para recibir a Cpu");
	clienteCpu = esperar_cliente(server_fd,"Cpu");

//	t_list* listaQueContieneNroTabla1erNivelYentrada = list_create();
//	t_list* listaQueContieneEntradaDeTabla2doNivel = list_create();
//	t_list* listaQueContieneDireccionFisca = list_create();
//	t_list* listaQueContieneValorAEscribir = list_create();
//	t_list* listaQueContieneDirFisica1YDirFisica2 = list_create();

	//int a = 1;
	while(1) {
		int cod_op = recibir_operacion(clienteCpu);
		//pthread_mutex_lock(&mutexMemoriaData);

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
//
//			case SEGUNDO_ACCESO://poner cases mas expresivos envio de marco
//				listaQueContieneEntradaDeTabla2doNivel = recibir_paquete_int(clienteCpu);
//				int entradaTabla2doNivel = (int) list_get(listaQueContieneEntradaDeTabla2doNivel,0);
//				log_info(logger,"Me llego  la entrada de segundo nivel %d",entradaTabla2doNivel);
//
//				int marco =  marcoSegunIndice(nroTabla2doNivel,entradaTabla2doNivel);
//
//				enviarMarco(clienteCpu,marco);
//
//				break;
//			case READ://caso: me envia dir fisica y leo el valor de esa direccion
//				listaQueContieneDireccionFisca = recibir_paquete_int(clienteCpu);
//
//				marco = (int) list_get(listaQueContieneDireccionFisca,0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
//				int desplazamiento = (int) list_get(listaQueContieneDireccionFisca,1);
//				int leer = (int) list_get(listaQueContieneDireccionFisca,2);
//
//				log_info(logger,"Me llego el marco %d con desplazamiento %d",marco,desplazamiento);
//
//				if(leer == 1){
//					log_info(logger,"-------------------READ-------------------");
//
//					uint32_t numeroALeer = leerElPedido(marco,desplazamiento);
//					log_info(logger, "Valor leido: %u",numeroALeer);
//
//					log_info(logger,"-------------------READ-------------------\n");
//				}
//
//				break;
//			case WRITE://caso: me envia dir fisica y escribo el valor en esa direccion
//				listaQueContieneValorAEscribir = recibir_paquete_int(clienteCpu);
//				uint32_t valorAEscribir = (uint32_t) list_get(listaQueContieneValorAEscribir,0);
//
//				log_info(logger,"-------------------WRITE-------------------");
//
//				log_info(logger,"Me llego el valor a escribir: %u",valorAEscribir);
//				escribirElPedido((uint32_t) valorAEscribir,marco,desplazamiento); //casteo para que no joda el warning
//
//				log_info(logger,"-------------------WRITE-------------------\n");
//				break;
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
				break;
			case -1:
				log_error(logger, "Se desconecto el cliente. Terminando conexion");
				return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}

		// pthread_mutex_unlock(&mutexMemoriaData);
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

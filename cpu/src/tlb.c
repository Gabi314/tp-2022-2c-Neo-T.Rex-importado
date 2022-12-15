#include "tlb.h"
t_list* tlb;
//int tamanioEntradaTlb = 0;
int contadorDeEntradasTlb = 0;

void agregarEntradaATLB(int marco, int pagina, int pid, int numeroDeSegmento){

	entradaTLB* unaEntrada = malloc(sizeof(entradaTLB));
	//tamanioEntradaTlb += sizeof(entradaTLB);

	unaEntrada->nroDeMarco = marco;
	unaEntrada->nroDePagina = pagina;
	unaEntrada->nroDeProceso = pid;
	unaEntrada->nroDeSegmento = numeroDeSegmento;
	unaEntrada->instanteGuardada = time(NULL);
	unaEntrada->ultimaReferencia = time(NULL);

	log_info(logger, "<PID: %d> Se agrega la pagina %d perteneciente al segmento %d con marco %d a tlb",
			pid, pagina, numeroDeSegmento, marco);

	list_add_in_index(tlb,contadorDeEntradasTlb,unaEntrada);
	contadorDeEntradasTlb++;

//	int i=0;
//
//	if(i<2){
		entradaTLB* unaEntradaAux;
		unaEntradaAux =	list_get(tlb,0);

		log_info(logger,"Numero de marco es: %d",unaEntradaAux->nroDeMarco);
		log_info(logger,"Numero de pagina es: %d",unaEntradaAux->nroDePagina);
//	}
//	i++;
		imprimirEntradasTLB();

	//free(unaEntrada); //ANTES CUANDO ESTABA ESTE FREE, EL IMPRIMIR TE TIRABA BASURA, AHORA LO QUE HACE ES TIRA REPETIDAS VECES LA ULTIMA ENTRADA CARGADA

}

void algoritmosDeReemplazoTLB(int pagina,int marco,int pid, int numeroDeSegmento){//probarrrr-----------------------------------
	if(! strcmp(algoritmoReemplazoTlb,"FIFO")){
		entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
		entradaTLB* otraEntradaTLB = malloc(sizeof(entradaTLB));

		int indiceConInstanteGuardadoMayor = 0;

		for(int i = 0; i<cantidadEntradasTlb;i++){
			unaEntradaTLB = list_get(tlb,i);
			otraEntradaTLB = list_get(tlb,indiceConInstanteGuardadoMayor);
			if(unaEntradaTLB->nroDeProceso == pid && otraEntradaTLB->nroDeProceso == pid){
				if(unaEntradaTLB->instanteGuardada < otraEntradaTLB->instanteGuardada){
					indiceConInstanteGuardadoMayor = i;
				}
			}
		}

		reemplazarPagina(pagina,marco,indiceConInstanteGuardadoMayor,numeroDeSegmento,pid);

	}else if (! strcmp(algoritmoReemplazoTlb,"LRU")){
		entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB)); //repito logica por la ultimaReferencia
		entradaTLB* otraEntradaTLB = malloc(sizeof(entradaTLB));

		int indiceConUltimaReferenciaMayor = 0;

		for(int i = 0; i<cantidadEntradasTlb;i++){
			unaEntradaTLB = list_get(tlb,i);
			otraEntradaTLB = list_get(tlb,indiceConUltimaReferenciaMayor);

			if(unaEntradaTLB->nroDeProceso == pid && otraEntradaTLB->nroDeProceso == pid){
				if(unaEntradaTLB->ultimaReferencia < otraEntradaTLB->ultimaReferencia){
					indiceConUltimaReferenciaMayor = i;
				}
			}
		}

		reemplazarPagina(pagina,marco,indiceConUltimaReferenciaMayor,numeroDeSegmento,pid);
	}
}

void reemplazarPagina(int pagina,int marco ,int indice, int numeroDeSegmento, int pid){//Repito logica con agregarEntradaTLB
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
	unaEntradaTLB = list_get(tlb,indice);

	log_info(logger,"TLB: Reemplazo la pagina %d en el marco %d cuya entrada es %d",pagina,marco,indice);

	unaEntradaTLB->nroDePagina = pagina;
	unaEntradaTLB->nroDeMarco = marco;
	unaEntradaTLB->nroDeSegmento = numeroDeSegmento;
	unaEntradaTLB->instanteGuardada = time(NULL);
	unaEntradaTLB->ultimaReferencia = time(NULL);
	unaEntradaTLB->nroDeProceso = pid;

	list_replace(tlb,indice,unaEntradaTLB);

	imprimirEntradasTLB();
}

void inicializarTLB(){

	tlb = list_create();
}

//Se debe limpiar cuando finalizan el proceso
void limpiarEntradasTLB(int pid){
	entradaTLB* unaEntradaTLB;

	for(int i = 0; i<list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);
		if(unaEntradaTLB->nroDeProceso == pid){
			list_remove(tlb,i);
			contadorDeEntradasTlb--;
		}
	}
}

void imprimirEntradasTLB(){
	entradaTLB* unaEntradaTLB; //= malloc(sizeof(entradaTLB));
	for(int i = 0; i < list_size(tlb); i++){
		unaEntradaTLB = list_get(tlb,i);
		log_info(logger,
				"<%d>|PID:<%d>|SEGMENTO:<%d>|PAGINA:<%d>|MARCO:<%d>"
				, i
				, unaEntradaTLB->nroDeProceso
				, unaEntradaTLB->nroDeSegmento
				, unaEntradaTLB->nroDePagina
				, unaEntradaTLB->nroDeMarco);
	}
	//free(unaEntradaTLB);
}


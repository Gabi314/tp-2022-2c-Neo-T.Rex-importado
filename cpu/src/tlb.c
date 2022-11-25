#include "tlb.h"
t_list* tlb;

void agregarEntradaATLB(int marco, int pagina, int pid, int numeroDeSegmento){

	entradaTLB* unaEntrada = malloc(sizeof(entradaTLB));

	unaEntrada->nroDeMarco = marco;
	unaEntrada->nroDePagina = pagina;
	unaEntrada->nroDeProceso = pid;
	unaEntrada->nroDeSegmento = numeroDeSegmento;
	unaEntrada->instanteGuardada = time(NULL);
	unaEntrada->ultimaReferencia = time(NULL);

	log_info(logger, "<PID: %d> Se agrega la pagina %d perteneciente al segmento %d con marco %d a tlb",
			pid, pagina, numeroDeSegmento, marco);

	list_add(tlb,unaEntrada);

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

		reemplazarPagina(pagina,marco,indiceConInstanteGuardadoMayor,numeroDeSegmento);

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

		reemplazarPagina(pagina,marco,indiceConUltimaReferenciaMayor,numeroDeSegmento);
	}
}

void reemplazarPagina(int pagina,int marco ,int indice, int numeroDeSegmento){//Repito logica con agregarEntradaTLB
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
	unaEntradaTLB = list_get(tlb,indice);

	log_info(logger,"TLB: Reemplazo la pagina %d en el marco %d cuya entrada es %d",pagina,marco,indice);

	unaEntradaTLB->nroDePagina = pagina;
	unaEntradaTLB->nroDeMarco = marco;
	unaEntradaTLB->nroDeSegmento = numeroDeSegmento;
	unaEntradaTLB->instanteGuardada = time(NULL);
	unaEntradaTLB->ultimaReferencia = time(NULL);

	list_replace(tlb,indice,unaEntradaTLB);

}

t_list* inicializarTLB(){

	tlb = list_create();
	return tlb;
}

//Se debe limpiar cuando finalizan el proceso
void reiniciarTLB(){
	list_clean(tlb);
}


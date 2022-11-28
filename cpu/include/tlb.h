#include "funcionesCpu.h"

void agregarEntradaATLB(int marco, int pagina, int pid, int numeroDeSegmento);

void reemplazarPagina(int pagina,int marco ,int indice, int numeroDeSegmento);

void algoritmosDeReemplazoTLB(int pagina,int marco,int pid, int numeroDeSegmento);

t_list* inicializarTLB();

void limpiarEntradasTLB(int pid);


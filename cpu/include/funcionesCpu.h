#ifndef FUNCIONES_CPU_H_
#define FUNCIONES_CPU_H_
#include <stdbool.h>
//--------------------INCLUDES

#include <shared/hello.h>
//--------------PUERTOS
//Revisar
#define IP_CPU "127.0.0.1"


//
//typedef enum//REVISAR LOS ENUM
//{
//	MENSAJE_A_KERNEL,
//	RECIBIR_PCB,
//	I_O,
//	EXIT,
//	INTERRUPT,
//	MENSAJE_INTERRUPT
//}op_code_kernel;

extern t_log* logger;
extern t_log* loggerObligatorio;
extern t_config* config;
extern t_paquete* paquete;

extern t_list* tlb;

//--------------  Cpu como servidor de Kernel ---------
extern int clienteKernel;
extern int clienteKernelInterrupt;
extern int server_fd;
extern char* ipMemoria;
extern char* puertoMemoria;

extern char* puertoDeEscuchaDispatch;
extern char* puertoDeEscuchaInterrupt;

extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int desplazamiento;
extern int socket_memoria;

//extern uint32_t ax;
//extern uint32_t bx;
//extern uint32_t cx;
//extern uint32_t dx;
extern bool ejecutando;
extern bool hayInterrupcion;

extern char* algoritmoReemplazoTlb;
extern int cantidadEntradasTlb;

typedef struct
{
	int nroDeProceso;
	int nroDeSegmento;
	int nroDePagina;
	int nroDeMarco;
	time_t instanteGuardada;
	time_t ultimaReferencia;
} entradaTLB;

typedef enum
{
	AX,
	BX,
	CX,
	DX
}registros;

typedef enum{
	SET,
	ADD,
	MOV_IN,
	MOV_OUT,
	IO,
	EXT
}identificadorInstruccion;

typedef enum
{
	PANTALLA = 0,
	TECLADO = 1
}dispositivos_IO;


//--------------------VARIABLES
extern t_log* logger;
extern t_config* config;

extern t_pcb* unPcb;
extern char** listaDispositivos;

//----------------------------DECLARO FUNCIONES
int chequeoCantidadArchivos(int);
void inicializarConfiguraciones(char*);
t_pcb* recibir_pcb(int);
void obtenerTamanioIdentificadores(instruccion*);
void agregarInstruccionesAlPaquete(instruccion*);

void obtenerCantidadDeSegmentos(entradaTablaSegmento*);
void agregarSegmentosAlPaquete(entradaTablaSegmento*);

uint32_t registroAUtilizar(int,t_registros);
int chequeoDeDispositivo(char*);

instruccion* buscarInstruccionAEjecutar(t_pcb*);
void leerTamanioDePaginaYCantidadDeEntradas(t_list*);
void ejecutar(instruccion*,t_pcb*);
int decode(instruccion*);
int buscarDireccionFisica(t_pcb*);
char* dispositivoIOSegunParametro(int);
int chequearMarcoEnTLB(int,int,int);
bool calculoDireccionLogicaExitoso(int,t_list*);
int accederAMemoria(int,int,t_pcb*);

bool haySegFault(int, int,t_list *);
void checkInterrupt();
int escucharInterrupciones();

t_list* inicializarTLB();
void reiniciarTLB();
void ejecucion();
void enviarNroTablaDePaginas(t_list*,int,int,int);
void enviarDireccionFisica(int,int,int,int);
void bloqueoPorPageFault(t_pcb*);

void iterator(instruccion*);

//----------------------------FUNCIONES DE CONEXIONES
int conexionConKernelDispatch(void);
int conexionConMemoria(void);
void enviar_pcb(t_pcb*,int,int);

extern pthread_t hiloInterrupciones;


#endif /*FUNCIONES_CPU_H_*/

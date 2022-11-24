#ifndef UTILS_H_
#define UTILS_H_

#include <shared/hello.h>

extern t_log* logger;
extern t_list* listaInstrucciones;
#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"

typedef enum
{
	DISPOSITIVO_E_S,
	PANTALLA,
	TECLADO
} dispositivos_IO;
/*
typedef struct
{
	int AX;
	int BX;
	int CX;
	int DX;
} t_registros;

typedef enum estado { NEW, READY, BLOCKED, EXEC, TERMINATED } t_estado;
typedef enum algoritmo { FIFO, RR } t_algoritmo_pcb;

typedef struct
{
	int idProceso;
	t_list* instrucciones;
	int programCounter;
	t_registros registros;
	t_list* tablaSegmentos;
	int socket;
	t_estado estado;
	t_algoritmo_pcb algoritmoActual;
} t_pcb;
*/
typedef struct
{
	t_pcb* pcb;
	int registro;
} t_info_teclado;

typedef struct {
	t_pcb* pcb;
	int registro;
} t_info_pantalla;

typedef struct {
	t_pcb* pcb;
	int pagina;
} t_info_pf;

// En principio, no hace falta ------------------------------------
typedef struct {
	char* dispositivo;
	int tiempo;
	t_queue* cola_procesos;
	t_queue* cola_UTs;
	sem_t semaforo;

} t_elem_disp;
// En principio, no hace falta ------------------------------------
/*
typedef struct
{
	char* identificador;
	int parametros[2];
} instruccion;

*/
/*
typedef struct
{
	int numeroSegmento;//revisar
	int tamanioSegmento;
	int numeroTablaPaginas;
}entradaTablaSegmento;
*/


extern int socketServidor;

t_list* recibir_paquete_instrucciones(int);

t_list * inicializar_tabla_segmentos(int);
void inicializar_registros(t_registros registros);

void inicializar_configuraciones(char* unaConfig);
void inicializar_listas_y_colas();
void inicializar_lista_dispositivos();
void iterator(instruccion*);


int get_identificador();
t_pcb* recibir_pcb(int socket_cliente);

void inicializar_configuraciones(char* unaConfig);
void inicializar_listas_y_colas();
void inicializar_lista_dispositivos();
void inicializar_colas();
void inicializar_semaforos();
void levantar_hilo_dispositivo(t_elem_disp*);

void iterator(instruccion*);


t_pcb * conexionConConsola();// cual usamos
int conexionConCpu(t_pcb * PCB);

void agregarANew(t_pcb* proceso);
t_pcb* sacarDeNew();

t_pcb* obtenerSiguienteDeReady();
t_pcb* obtenerSiguienteFIFO();
t_pcb* obtenerSiguienteRR();
void ejecutar(t_pcb* proceso);

void recibir_consola(int * servidor) ;
void atender_consola(int * nuevo_cliente);
void asignar_memoria();
void readyAExe();
void atender_interrupcion_de_ejecucion();
void atender_IO_teclado(t_info_teclado * info);
void atender_IO_pantalla(t_info_pantalla * info);
void atender_IO_generico(t_elem_disp*);
void controlar_quantum();
void atender_page_fault(t_info_pf*);


extern char * ipMemoria;
extern char * puertoMemoria;
extern char * ipCpu;
extern char * puertoCpuDispatch;
extern char * puertoCpuInterrupt;
extern char * puertoKernel;
extern char * algoritmoPlanificacion;
extern int gradoMultiprogramacionTotal;
extern char** dispositivos_io;
extern char** tiempos_io;
extern t_list colas_dispositivos_io;
extern int quantum_rr;

extern sem_t kernelSinFinalizar;

extern int conexionCpu;
extern int conexionCpuInterrupt;
extern int socketMemoria;
extern int tamanioTotalIdentificadores;
extern int contadorInstrucciones;
extern int contadorSegmentos;
extern int desplazamiento;


extern t_queue* colaNew;
extern t_queue* colaReadyFIFO;
extern t_queue* colaReadyRR;
//extern t_queue* colaBlockedPantalla; No hacen falta, cada consola tiene su pantalla y teclado
//extern t_queue* colaBlockedTeclado;
extern t_list* listaDeColasDispositivos;

extern t_pcb* pcbTeclado;
extern int registroTeclado;
extern t_pcb* pcbPantalla;
extern int registroPantalla;

extern int identificadores_pcb;

extern pthread_t hiloQuantumRR;

extern sem_t kernelSinFinalizar;
extern sem_t gradoDeMultiprogramacion;
extern sem_t cpuDisponible;
extern sem_t pcbEnNew;
extern sem_t pcbEnReady;
extern pthread_mutex_t mutexNew;
extern pthread_mutex_t obtenerProceso;
extern pthread_mutex_t mutexPantalla;
extern pthread_mutex_t mutexTeclado;

void obtenerTamanioIdentificadores(instruccion*);
void obtenerCantidadDeSegmentos(entradaTablaSegmento*);
void agregarInstruccionesAlPaquete(instruccion*);
void agregarSegmentosAlPaquete(entradaTablaSegmento*);
void agregar_a_paquete_kernel_cpu(t_pcb*,int,int);
void aplanarDispositivosIO(char**);



void destruirProceso(t_pcb*);

#endif /* UTILS_H_ */

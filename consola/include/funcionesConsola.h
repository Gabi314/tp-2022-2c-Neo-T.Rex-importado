#ifndef UTILS_H_
#define UTILS_H_

#include <shared/hello.h>

typedef enum
{
	DISCO = 0,
	PANTALLA = 1,
	TECLADO = 2
}dispositivos_IO;

typedef enum
{
	AX,
	BX,
	CX,
	DX
}registros;


//Estructura auxiliar de parametros, por las dudas
typedef struct
{
	int valor;
	char* registro;
	char* otroRegistro;
}parametro;


typedef struct
{
	char* identificador;
	//parametro* parametros;
	int parametros[2];
} instruccion;

//------------------ DECLARACION
extern t_log* logger;
extern t_config* config;
extern char* ipKernel;
extern char* puertoKernel;
extern char** segmentos;
extern int tiempoPantalla;
extern int conexion;


//------------------ DECLARACION FUNCIONES DE CONEXIONES

void agregar_a_paquete_instrucciones(t_paquete*, instruccion*, int);
void enviarPaqueteTamanioDeSegmentos();
void enviarListaInstrucciones(t_paquete*);

void agregarAPaqueteSegmentos(char**,t_paquete*);
void dividirInstruccionesAlPaquete(t_log*,t_paquete*,char**,instruccion*);

int chequeoCantidadArchivos(int);
void inicializarConfiguraciones(char*);

//------------------ DECLARACION FUNCIONES
FILE* abrirArchivo(char*);
FILE* recorrerArchivo(char*,FILE*);

void imprimirValorPorPantalla(int);
void solicitudIngresarValorPorTeclado(int);

int chequeoDeRegistro(char*);
int chequeoDeDispositivo(char*);
void atenderPeticionesKernel();


#endif /* UTILS_H_ */

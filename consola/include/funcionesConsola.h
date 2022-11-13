#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <shared/hello.h>

typedef enum
{
	PANTALLA = 0,
	TECLADO = 1
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

extern char** listaDispositivos;


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
int parametroIOSegunDispositivo(char*);
void atenderPeticionesKernel();


#endif /* UTILS_H_ */

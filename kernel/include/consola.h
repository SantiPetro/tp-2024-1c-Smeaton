#ifndef CONSOLA_H_
#define CONSOLA_H_
#include <../include/protocolo.h>
#include <pthread.h>
#include <../include/serializacion.h>
#include <readline/readline.h>
#include <commons/string.h>
#include <../include/proceso.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    op_code opcode;
    char* path;
} iniciar_proceso_t;

typedef struct {
    t_log* logger;
    int socket;
} leer_consola_t;

//pthread_t hilo_procesar_instruccion;
void leer_consola(void* args_void);
void procesar_instruccion(char* instruccion, t_log* logger, int socket);
void empezar_hilo_consola(pthread_t* hilo_consola, t_log* logger,int socket);
void enviar_inicio_proceso(int socket, char* path, t_log* logger);
void ejecutar_script(char* path, t_log* logger, int socket);
bool existe_archivo(char* path);
void listar_procesos();
#endif
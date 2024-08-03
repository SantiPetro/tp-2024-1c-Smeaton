#ifndef PROCESOS_H_
#define PROCESOS_H_

#include <stdio.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>

typedef struct {
    char* path;
    uint32_t pid;
} archivo_proceso_t;

bool existe_archivo(char* path);
void agregar_proceso(t_list* archivos_procesos, char* path, uint32_t pid);
char* buscar_instruccion(uint32_t pid, uint32_t pc, t_list* archivos_procesos);
char* buscar_instruccion_en(FILE* f, uint32_t pc);
void archivo_proceso_destroy(archivo_proceso_t* archivo_proceso);
archivo_proceso_t* archivo_proceso_por_pid(uint32_t pid, t_list* archivos_procesos);
void eliminar_archivo_proceso(t_list* archivos_procesos, uint32_t pid_a_finalizar);
extern int retardo_respuesta;
extern pthread_mutex_t mutex_archivo_proceso;

#endif
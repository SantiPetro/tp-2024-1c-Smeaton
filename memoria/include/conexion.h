#ifndef CONEXION_H_
#define CONEXION_H_

#include <../include/sockets.h>
#include <../include/protocolo.h>
#include <../include/archivo_proceso.h>
#include <../include/conversores.h>
#include <../include/paginas.h>
#include <commons/bitarray.h>

void procesar_conexion(void* args);
void enviar_pid(int socket_cliente, uint32_t pid);
void enviar_instruccion(int socket, char* instruccion);
void enviar_out_of_memory(int socket_cliente);
void enviar_ok(int socket_cliente);
extern t_list* archivos_procesos;
extern void* memoria;
extern t_list* tablas_paginas_memoria;
extern int tam_memoria;
extern uint32_t tam_pagina;
extern t_bitarray* bitarray_tabla;
extern pthread_mutex_t mutex_memoria;
extern pthread_mutex_t mutex_paginas;
extern pthread_mutex_t mutex_archivo_proceso;
extern pthread_mutex_t mutex_bit_array;

#endif
#ifndef UTILS_SOCKETS_H_
#define UTILS_SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <pthread.h>


typedef void* (*procesar_conexion_func_t)(void*);
typedef struct {
    int socket_cliente;
    t_log* logger;
} conexion_args_t;

typedef struct 
{
    int socket_server;
    t_log* logger;
    procesar_conexion_func_t procesar_conexion_func;
    char* nombre_server;
    int* flag;
} server_hilo_args_t;

int iniciar_servidor(t_log* logger, char* puerto, char* nombre_server);
int esperar_cliente(int socket_servidor, t_log* logger, char* nombre_server);
int server_escuchar(int socket_server, t_log* logger, procesar_conexion_func_t procesar_conexion_func, char* nombre_server);
int crear_conexion(t_log* logger, const char* nombre_server, char* ip, char* puerto);
int generar_conexion(t_log* logger, char* nombre_server, char* ip, char* puerto, t_config* config);

#endif
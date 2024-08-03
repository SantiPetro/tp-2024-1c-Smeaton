#ifndef SENIALES_H
#define SENIALES_H

#include <stdlib.h>
#include <stdio.h>
#include <../include/serializacion.h>
#include <signal.h>
#include <../include/protocolo.h>
#include <commons/log.h>

extern char* tipo_interfaz;
extern int kernel_fd;
extern char* nombre;
void controlar_seniales(t_log* logger);
void cerrar_seniales();
void avisar_desconexion_kernel();

#endif
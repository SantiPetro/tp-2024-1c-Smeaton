#ifndef SENIALES_H
#define SENIALES_H

#include <signal.h>
#include <commons/collections/list.h>
#include <../include/archivo_proceso.h>
#include <commons/log.h>

void controlar_seniales(t_log* logger);
void cerrar_seniales();
extern t_list* archivos_procesos;

#endif
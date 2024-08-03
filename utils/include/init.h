#ifndef UTILS_INIT_H_
#define UTILS_INIT_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>

t_log* iniciar_logger(char* nombre_archivo, char* nombre_proceso);
t_config* iniciar_config(char* ruta);
void terminar_programa(t_log *logger, t_config *config); 

#endif

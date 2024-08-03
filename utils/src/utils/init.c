#include <../include/init.h>

t_log* iniciar_logger(char* nombre_archivo, char* nombre_proceso) {
   t_log *nuevo_logger;
   if ((nuevo_logger = log_create(nombre_archivo, nombre_proceso, 1, LOG_LEVEL_INFO))
           == NULL) {
       printf("No pude crear el logger\n");
       exit(1); //Chequear si es mejor el abort
   }

   return nuevo_logger;
}

t_config* iniciar_config(char* ruta) {
   t_config *nuevo_config;
   if ((nuevo_config = config_create(ruta)) == NULL) {
       printf("No se pudo crear el config");
       exit(2); //Chequear si es mejor el abort
   }
   return nuevo_config;
}

void terminar_programa(t_log *logger, t_config *config) {
   log_destroy(logger);
   config_destroy(config);
}
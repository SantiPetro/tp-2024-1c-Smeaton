#include <../include/main.h>

void get_config(t_config* config) {
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = (uint32_t)config_get_int_value(config, "TAM_PAGINA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
}


void inicializar_semaforos()
{
    pthread_mutex_init(&mutex_memoria, NULL);
    pthread_mutex_init(&mutex_paginas, NULL);
    pthread_mutex_init(&mutex_archivo_proceso, NULL);
    pthread_mutex_init(&mutex_bit_array, NULL);
}

void liberar_semaforos()
{
    pthread_mutex_destroy(&mutex_memoria);
    pthread_mutex_destroy(&mutex_paginas);
    pthread_mutex_destroy(&mutex_archivo_proceso);
    pthread_mutex_destroy(&mutex_bit_array);
}

int main(int argc, char* argv[]) {
    logger_memoria = iniciar_logger("memoria.log", "MEMORIA: ");
    t_config* config_memoria = iniciar_config(argv[1]);
    get_config(config_memoria);

    archivos_procesos = list_create();

    inicializar_semaforos();

    //PAGINAS
    memoria = malloc(tam_memoria);
    tablas_paginas_memoria = list_create();
    int cant_marcos = cantidad_marcos();
    char* bitarray = malloc((cant_marcos+7)/8);
    memset(bitarray, 0, (cant_marcos+7)/8);
    bitarray_tabla = bitarray_create(bitarray,(cant_marcos+7)/8);

    

    //Empieza el servidor
    int memoria_fd = iniciar_servidor(logger_memoria, puerto_escucha, "memoria");
    while(server_escuchar(memoria_fd, logger_memoria, (procesar_conexion_func_t)procesar_conexion, "memoria"));

    liberar_semaforos();
    terminar_programa(logger_memoria, config_memoria);
    return 0;
}
#ifndef PAGINAS_H_
#define PAGINAS_H_

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <pthread.h>

typedef struct
{
    int nro_pagina;
    uint16_t marco;
    uint8_t bytes_ocupados;
} pagina_t;

typedef struct {
    int cantidad_paginas;
    t_list* paginas;
    uint32_t pid;
} tabla_t;

tabla_t* iniciar_tabla(uint32_t pid, int cantidad_paginas);
void pagina_destroy(pagina_t* pagina);
int cantidad_marcos();
int tamanio_proceso(uint32_t pid);
tabla_t* tabla_paginas_por_pid(uint32_t pid);
bool ampliar_tamanio_proceso(uint32_t pid, int tamanio);
void reducir_tamanio_proceso(uint32_t pid, int tamanio);	
int completar_ultima_pagina(tabla_t* tabla, int tamanio);
bool tiene_validez(pagina_t* pagina);
int vaciar_ultima_pagina(tabla_t* tabla, int tamanio);
void eliminar_tabla(uint32_t pid);
int cantidad_paginas_proceso(uint32_t pid_a_finalizar);
pagina_t *buscar_pagina_por_nro(tabla_t *tabla, int nro_pagina);
bool pagina_por_nro(pagina_t *pagina);
void escribir(uint16_t dir_fis, void* valor, uint16_t cantidad_de_bytes, t_log* logger, uint32_t pid);
void leer(void* lectura, uint16_t dir_fis, uint16_t cantidad_de_bytes, t_log* logger, uint32_t pid);
extern t_list* tablas_paginas_memoria;
extern int tam_memoria;
extern uint32_t tam_pagina;
extern t_bitarray* bitarray_tabla;
extern void* memoria;
extern pthread_mutex_t mutex_memoria;
extern pthread_mutex_t mutex_paginas;
extern pthread_mutex_t mutex_archivo_proceso;
extern pthread_mutex_t mutex_bit_array;
extern t_log* logger_memoria;

#endif
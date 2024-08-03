#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct
{
    uint32_t pid;
    int nro_pagina;
    uint16_t marco;
    uint16_t llegada;
} fila_tlb_t;

typedef struct {
    t_list* filas;
    uint16_t proxima_llegada;
} tlb_t;

extern tlb_t* tlb;
extern int cantidad_entradas_tlb;
extern t_queue* cola_fifo_tlb;
extern char* algoritmo_tlb;

void agregar_a_tlb(uint32_t nro_pagina, uint16_t marco, uint32_t pid);
bool tlb_llena();
int buscar_marco_tlb(uint32_t nro_pagina, uint32_t pid);
void inicializar_tlb();
void agregar_segun_algoritmo(uint32_t nro_pagina, uint16_t marco, uint32_t pid);
void reemplazar_segun_algoritmo(uint32_t nro_pagina, uint16_t marco, uint32_t pid);
static void* fila_menor_llegada(fila_tlb_t* a, fila_tlb_t* b);
static void fila_destroy(fila_tlb_t* fila);
void eliminar_tlb();

#endif
#include <../include/tlb.h>

void agregar_a_tlb(uint32_t nro_pagina, uint16_t marco, uint32_t pid)
{
    if (cantidad_entradas_tlb)
    {
        if (tlb_llena())
        {
            reemplazar_segun_algoritmo(nro_pagina, marco, pid);
        }
        else
        {
            agregar_segun_algoritmo(nro_pagina, marco, pid);
        }
    }
}

bool tlb_llena()
{
    return list_size(tlb->filas) == cantidad_entradas_tlb;
}

int buscar_marco_tlb(uint32_t nro_pagina, uint32_t pid)
{

    bool buscar_marco_por_pid_pagina(fila_tlb_t * fila)
    {
        return fila->nro_pagina == nro_pagina && fila->pid == pid;
    }
    fila_tlb_t *fila = list_find(tlb->filas, (void *)buscar_marco_por_pid_pagina);
    if (strcmp(algoritmo_tlb, "LRU") == 0 && fila != NULL)
    {
        fila->llegada = tlb->proxima_llegada++;
    }
    if (fila == NULL)
    {
        return -1;
    }

    return fila->marco;
}

void inicializar_tlb()
{
    tlb = malloc(sizeof(tlb_t));
    tlb->filas = list_create();
    tlb->proxima_llegada = 0;
    cola_fifo_tlb = queue_create();
}

void agregar_segun_algoritmo(uint32_t nro_pagina, uint16_t marco, uint32_t pid)
{
    fila_tlb_t *fila = malloc(sizeof(fila_tlb_t));
    fila->marco = marco;
    fila->nro_pagina = nro_pagina;
    fila->pid = pid;
    if (strcmp(algoritmo_tlb, "FIFO") == 0)
    {
        fila->llegada = 0;
        list_add(tlb->filas, fila);
        queue_push(cola_fifo_tlb, fila);
    }
    if (strcmp(algoritmo_tlb, "LRU") == 0)
    {
        fila->llegada = tlb->proxima_llegada++;
        list_add(tlb->filas, fila);
    }
}

void reemplazar_segun_algoritmo(uint32_t nro_pagina, uint16_t marco, uint32_t pid)
{
    fila_tlb_t *fila = malloc(sizeof(fila_tlb_t));
    fila->marco = marco;
    fila->nro_pagina = nro_pagina;
    fila->pid = pid;
    if (strcmp(algoritmo_tlb, "FIFO") == 0)
    {
        fila->llegada = 0;
        fila_tlb_t *primero = queue_peek(cola_fifo_tlb);
        list_remove_element(tlb->filas, primero);
        free(primero);
        list_add(tlb->filas, fila);
        queue_pop(cola_fifo_tlb);
        queue_push(cola_fifo_tlb, fila);
    }
    if (strcmp(algoritmo_tlb, "LRU") == 0)
    {
        fila_tlb_t *fila_menor = (fila_tlb_t *)list_get_minimum(tlb->filas, (void *)fila_menor_llegada);
        list_remove_element(tlb->filas, fila_menor);
        free(fila_menor);
        fila->llegada = tlb->proxima_llegada++;
        list_add(tlb->filas, fila);
    }
}

static void *fila_menor_llegada(fila_tlb_t *a, fila_tlb_t *b)
{
    return a->llegada < b->llegada ? a : b;
}

void eliminar_tlb()
{
    list_destroy_and_destroy_elements(tlb->filas, (void *)fila_destroy);
    free(tlb);
    queue_destroy_and_destroy_elements(cola_fifo_tlb, free);
}

static void fila_destroy(fila_tlb_t *fila)
{
    free(fila);
}
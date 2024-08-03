#include <../include/paginas.h>
int nro_pagina_a_buscar;

tabla_t *iniciar_tabla(uint32_t pid, int cantidad_paginas)
{
    tabla_t *tabla = malloc(sizeof(tabla_t));
    tabla->paginas = list_create();
    tabla->pid = pid;
    tabla->cantidad_paginas = 0;
    for (int i = 0; i < cantidad_paginas; i++)
    {
        pagina_t *pagina = malloc(sizeof(pagina));
        pagina->marco = i;
        pagina->bytes_ocupados = 0;
        pagina->nro_pagina = -1;
        list_add(tabla->paginas, pagina);
    }
    return tabla;
}

void pagina_destroy(pagina_t *pagina)
{
    free(pagina);
}

int cantidad_marcos()
{
    return tam_memoria / tam_pagina;
}

int tamanio_proceso(uint32_t pid)
{
    int tamanio_proceso = 0;
    tabla_t *tabla_proceso = tabla_paginas_por_pid(pid);
    if(!tabla_proceso->cantidad_paginas) {
        return 0;
    } else {
        pagina_t* ultima_pagina = buscar_pagina_por_nro(tabla_proceso, tabla_proceso->cantidad_paginas - 1);
        tamanio_proceso += ultima_pagina->bytes_ocupados;
        tamanio_proceso += (tabla_proceso->cantidad_paginas-1)*tam_pagina;
    }
    return tamanio_proceso;
}

tabla_t *tabla_paginas_por_pid(uint32_t pid)
{
    bool buscar_pagina(tabla_t *tabla_b)
    {
        return (tabla_b->pid == pid);
    }
    return list_find(tablas_paginas_memoria, (void *)buscar_pagina);
}



bool ampliar_tamanio_proceso(uint32_t pid, int tamanio)
{
    tabla_t *tabla_proceso = tabla_paginas_por_pid(pid);
    int tamanio_restante = completar_ultima_pagina(tabla_proceso, tamanio);

    int cant_marcos_otorgar = (tam_pagina + tamanio_restante - 1) / tam_pagina;

    for (int i = 0; (i < cantidad_marcos()) && cant_marcos_otorgar; i++)
    {
        pthread_mutex_lock(&mutex_bit_array);
        if (!bitarray_test_bit(bitarray_tabla, i))
        {
            pthread_mutex_unlock(&mutex_bit_array);
            pagina_t *pagina = list_get(tabla_proceso->paginas, i);
            tabla_proceso->cantidad_paginas++;
            pagina->nro_pagina = tabla_proceso->cantidad_paginas - 1;
            pthread_mutex_lock(&mutex_bit_array);
            bitarray_set_bit(bitarray_tabla, i);
            pthread_mutex_unlock(&mutex_bit_array);
            if (cant_marcos_otorgar == 1)
            {
                pagina->bytes_ocupados = tamanio_restante;
            }
            else
            {
                pagina->bytes_ocupados = tam_pagina;
            }
            cant_marcos_otorgar--;
            tamanio_restante -= tam_pagina;
        } else {
            pthread_mutex_unlock(&mutex_bit_array);
        }
    }
    return cant_marcos_otorgar;
}

int completar_ultima_pagina(tabla_t *tabla, int tamanio)
{
    if (!tabla->cantidad_paginas)
    {
        return tamanio;
    }
    pagina_t *pagina = buscar_pagina_por_nro(tabla, tabla->cantidad_paginas - 1);
    int espacio_restante = tam_pagina - pagina->bytes_ocupados;
    pthread_mutex_lock(&mutex_bit_array);
    bitarray_set_bit(bitarray_tabla, pagina->marco);
    pthread_mutex_unlock(&mutex_bit_array);
    if (tamanio <= espacio_restante)
    {
        pagina->bytes_ocupados += tamanio;
        return 0;
    }
    else
    {
        pagina->bytes_ocupados = tam_pagina;
        return tamanio - espacio_restante;
    }
}

pagina_t *buscar_pagina_por_nro(tabla_t *tabla, int nro_pagina)
{
    bool pagina_por_nro(pagina_t *pagina)
    {
        return (pagina->nro_pagina == nro_pagina);
    }
    return list_find(tabla->paginas, (void *)pagina_por_nro);
}

void reducir_tamanio_proceso(uint32_t pid, int tamanio)
{
    tabla_t *tabla_proceso = tabla_paginas_por_pid(pid);
    pagina_t* primera_pagina = list_get(tabla_proceso->paginas, 0); 
    pagina_t* segunda_pagina = list_get(tabla_proceso->paginas, 1);
    tamanio = vaciar_ultima_pagina(tabla_proceso, tamanio);
    for (int i = tabla_proceso->cantidad_paginas - 1; (i >= 0) && tamanio > 0; i--)
    {
        pagina_t *pagina = list_get(tabla_proceso->paginas, i);
        if (pagina->bytes_ocupados)
        {
            if (tamanio >= tam_pagina)
            {
                pagina->bytes_ocupados = 0;
                pagina->nro_pagina = -1;
                tabla_proceso->cantidad_paginas--;
                pthread_mutex_lock(&mutex_bit_array);
                bitarray_clean_bit(bitarray_tabla, i);
                pthread_mutex_unlock(&mutex_bit_array);
            }
            else
            {
                pagina->bytes_ocupados -= tamanio;
            }
            tamanio -= tam_pagina;
        }
    }
}

bool tiene_validez(pagina_t *pagina)
{
    return pagina->bytes_ocupados;
}

int vaciar_ultima_pagina(tabla_t *tabla, int tamanio)
{
    if (!tabla->cantidad_paginas)
    {
        return tamanio;
    }
    pagina_t *pagina = buscar_pagina_por_nro(tabla, tabla->cantidad_paginas - 1);
    if (tamanio >= pagina->bytes_ocupados)
    {
        int restante = tamanio - pagina->bytes_ocupados;
        pagina->bytes_ocupados = 0;
        pagina->nro_pagina = -1;
        tabla->cantidad_paginas--;
        pthread_mutex_lock(&mutex_bit_array);
        bitarray_clean_bit(bitarray_tabla, pagina->marco);
        pthread_mutex_unlock(&mutex_bit_array);
        return restante;
    }
    else
    {
        pagina->bytes_ocupados -= tamanio;
        return 0;
    }
}

void eliminar_tabla(uint32_t pid)
{
    tabla_t *tabla = tabla_paginas_por_pid(pid);
    list_destroy_and_destroy_elements(tabla->paginas, (void *)pagina_destroy);
    list_remove_element(tablas_paginas_memoria, tabla);
    free(tabla);
}

int cantidad_paginas_proceso(uint32_t pid_a_finalizar)
{
    return (tamanio_proceso(pid_a_finalizar) + tam_pagina - 1) / tam_pagina;
}

void escribir(uint16_t dir_fis, void* valor, uint16_t cantidad_de_bytes, t_log* logger, uint32_t pid) {
    log_info(logger, "PID: <%d> - Accion: <ESCRIBIR> - Direccion fisica: <%d> - Tamaño <%d>",
    pid, dir_fis, cantidad_de_bytes);
    pthread_mutex_lock(&mutex_memoria);
    memcpy(memoria + dir_fis, valor, cantidad_de_bytes);
    pthread_mutex_unlock(&mutex_memoria);
}

void leer(void* lectura, uint16_t dir_fis, uint16_t cantidad_de_bytes, t_log* logger, uint32_t pid) {
    log_info(logger, "PID: <%d> - Accion: <LEER> - Direccion fisica: <%d> - Tamaño <%d>",
    pid, dir_fis, cantidad_de_bytes);
    pthread_mutex_lock(&mutex_memoria);
    void* posicion = (char*)memoria + dir_fis;
    memcpy(lectura, posicion, cantidad_de_bytes);
    pthread_mutex_unlock(&mutex_memoria);
}



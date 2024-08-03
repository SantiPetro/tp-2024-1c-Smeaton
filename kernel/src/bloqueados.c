#include <../include/bloqueados.h>
/*
void enviar_proceso_io_gen_sleep(proceso_t *proceso, char *interfaz_sleep, uint32_t uni_de_trabajo)
{
    if (!strcmp("GENERICA", interfaz_sleep))
    {
        proceso_sleep_t *proceso_a_sleep = malloc(sizeof(proceso_sleep_t));
        proceso_a_sleep->proceso = proceso;
        proceso_a_sleep->uni_de_trabajo = uni_de_trabajo;
        pthread_mutex_lock(&mutex_generica_list);
        list_add(pcbs_generica, proceso_a_sleep);
        pthread_mutex_unlock(&mutex_generica_list);
        sem_post(&pcb_esperando_generica);
    }
    else
    {
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERFAZ INCORRECTA", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
    }
}
*/

int estaConectada(char *interfaz)
{
    return dictionary_has_key(diccionario_interfaces, interfaz);
}
/*
void hacer_io_gen_sleep()
{
        sem_wait(&pcb_esperando_generica);
        pthread_mutex_lock(&mutex_generica_exec);
        pthread_mutex_lock(&mutex_generica_list);
        proceso_sleep_t* proceso_a_sleep = list_get(pcbs_generica, 0);
        pthread_mutex_unlock(&mutex_generica_list);
    if (estaConectada("GENERICA"))
    {
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t));
        int offset = 0;
        agregar_opcode(stream, &offset, IO_GEN_SLEEP);
        log_info(logger_kernel, "UNIS: %d", proceso_a_sleep->uni_de_trabajo);
        agregar_uint32_t(stream, &offset, proceso_a_sleep->uni_de_trabajo);
        int socket_generica = (int)dictionary_get(diccionario_interfaces, "GENERICA");
        send(socket_generica, stream, offset, 0);
        free(stream);
        sem_wait(&vuelta_io_gen_sleep);
        recibir_fin_de_sleep();
    }
    else
    {
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERFAZ NO CONECTADA", proceso_a_sleep->proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso_a_sleep->proceso->pid);
        proceso_t* proceso = proceso_a_sleep->proceso;
        free(proceso_a_sleep);
        pthread_mutex_unlock(&mutex_generica_exec);
        entrar_a_exit(proceso);
    }
}
*/

void volver_a_ready(proceso_t *proceso)
{
    if (proceso->quantum == quantum)
    {
        pthread_mutex_lock(&mutex_ready_list);
        list_add(pcbs_ready, proceso);
        mostrar_pids_cola(pcbs_ready, "READY");
        pthread_mutex_unlock(&mutex_ready_list);
        // sem_post(&pcb_esperando_exec);
        ingresar_a_exec();
    }
    else
    {
        pthread_mutex_lock(&mutex_ready_prioritario_list);
        list_add(pcbs_ready_prioritarios, proceso);
        mostrar_pids_cola(pcbs_ready_prioritarios, "READY PRIORITARIO");
        pthread_mutex_unlock(&mutex_ready_prioritario_list);
        // sem_post(&pcb_esperando_exec);
        ingresar_a_exec();
    }
}

void enviar_proceso_a_wait(proceso_t *proceso, char *recurso_wait, uint32_t tiempo_en_cpu, t_temporal *timer)
{
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pid == pid_proceso;
    }
    if (existe_recurso(recurso_wait))
    {
        if (hay_recursos_de(recurso_wait))
        {
            pedir_recurso(recurso_wait, proceso, pid_proceso);
            if (!list_any_satisfy(pids_eliminados, _pids_iguales))
            {
                volver_a_exec(proceso, tiempo_en_cpu, timer);
            }
        }
        else
        {
            pthread_mutex_lock(&mutex_recursos_list[posicion_de_recurso(recurso_wait)]);
            list_add(lista_de_recurso(recurso_wait), proceso);
            pthread_mutex_unlock(&mutex_recursos_list[posicion_de_recurso(recurso_wait)]);
            void *stream = malloc(sizeof(op_code));
            int offset = 0;
            agregar_opcode(stream, &offset, BLOQUEADO_RECURSO);
            send(cpu_dispatch_fd, stream, offset, 0);
            free(stream);
            temporal_stop(timer);
            if (!strcmp(algoritmo_planificacion, "VRR"))
            {
                if (temporal_gettime(timer) - tiempo_en_cpu < proceso->quantum)
                {
                    proceso->quantum -= temporal_gettime(timer) - tiempo_en_cpu;
                }
                else
                {
                    proceso->quantum = quantum;
                }
            }
            temporal_destroy(timer);
            liberar_cpu();
            log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
            log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, recurso_wait);
            pedir_recurso(recurso_wait, proceso, pid_proceso);
            if (!list_any_satisfy(pids_eliminados, _pids_iguales))
            {
                verificar_detencion_de_planificacion();
                log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", proceso->pid);
                volver_a_ready(proceso);
            }
        }
    }
    else
    {
        void *stream = malloc(sizeof(op_code));
        int offset = 0;
        agregar_opcode(stream, &offset, RECURSO_INVALIDO);
        send(cpu_dispatch_fd, stream, offset, 0);
        free(stream);
        if (!strcmp("VRR", algoritmo_planificacion) || !strcmp("RR", algoritmo_planificacion))
        {
            esperar_llegada_de_proceso_rr_vrr(proceso, timer, logger_kernel);
        }
        else
        {
            esperar_llegada_de_proceso_fifo(proceso, logger_kernel, timer);
        }
    }
}

int posicion_de_recurso(char *recurso_wait)
{
    for (int i = 0; i < cantidad_recursos; i++)
    {
        if (!strcmp(recurso_wait, recursos[i]))
        {
            return i;
        }
    }
}

t_list *lista_de_recurso(char *recurso)
{
    return pcbs_recursos[posicion_de_recurso(recurso)];
}

bool hay_recursos_de(char *recurso)
{
    pthread_mutex_lock(&mutex_recursos_instancias[posicion_de_recurso(recurso)]);
    bool hay_recursos = instancias_recursos[posicion_de_recurso(recurso)] > 0;
    pthread_mutex_unlock(&mutex_recursos_instancias[posicion_de_recurso(recurso)]);
    return hay_recursos;
}

bool existe_recurso(char *recurso)
{
    for (int i = 0; i < cantidad_recursos; i++)
    {
        if (!strcmp(recurso, recursos[i]))
        {
            return true;
        }
    }
    return false;
}

bool pids_iguales(uint32_t pid1, uint32_t pid2)
{
    return pid1 == pid2;
}

void pedir_recurso(char *recurso_wait, proceso_t *proceso, uint32_t pid_proceso)
{
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }

    int indice = posicion_de_recurso(recurso_wait);
    int pid_ingresado = proceso->pid;
    sem_wait(&pcb_esperando_recurso[indice]);
    pthread_mutex_lock(&mutex_recursos_list[posicion_de_recurso(recurso_wait)]);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        list_remove_element(lista_de_recurso(recurso_wait), proceso);
        pthread_mutex_unlock(&mutex_recursos_list[posicion_de_recurso(recurso_wait)]);
        pthread_mutex_lock(&mutex_recursos_instancias[indice]);
        instancias_recursos[indice]--;
        pthread_mutex_unlock(&mutex_recursos_instancias[indice]);
        proceso->recursos[indice]++;
    }
    else
    {
        pthread_mutex_unlock(&mutex_recursos_instancias[indice]);
        sem_post(&pcb_esperando_recurso[indice]);
    }
}

void enviar_proceso_a_signal(proceso_t *proceso, char *recurso_signal, uint32_t tiempo_en_cpu, t_temporal *timer)
{
    if (existe_recurso(recurso_signal))
    {
        devolver_recurso(recurso_signal, proceso);
        volver_a_exec(proceso, tiempo_en_cpu, timer);
    }
    else
    {
        void *stream = malloc(sizeof(op_code));
        int offset = 0;
        agregar_opcode(stream, &offset, RECURSO_INVALIDO);
        log_info(logger_kernel, "OPCODE: %d", *((int *)stream));
        send(cpu_dispatch_fd, stream, offset, 0);
        free(stream);
        if (!strcmp("VRR", algoritmo_planificacion) || !strcmp("RR", algoritmo_planificacion))
        {
            esperar_llegada_de_proceso_rr_vrr(proceso, timer, logger_kernel);
        }
        else
        {
            esperar_llegada_de_proceso_fifo(proceso, logger_kernel, timer);
        }
    }
}

void devolver_recurso(char *recurso_signal, proceso_t *proceso)
{
    int indice = posicion_de_recurso(recurso_signal);
    pthread_mutex_lock(&mutex_recursos_instancias[indice]);
    instancias_recursos[indice]++;
    pthread_mutex_unlock(&mutex_recursos_instancias[indice]);
    if (proceso->recursos[indice] > 0)
    {
        proceso->recursos[indice]--;
    }
    sem_post(&pcb_esperando_recurso[indice]);
}

void enviar_proceso_a_interfaz(proceso_a_interfaz_t *proceso_a_interfaz, char *tipo_interfaz, void (*hacer_peticion)(proceso_a_interfaz_t *, interfaz_t *))
{
    interfaz_t *interfaz_solicitada = buscar_interfaz(proceso_a_interfaz->interfaz);
    if (interfaz_solicitada != NULL)
    {
        if (estaConectada(proceso_a_interfaz->interfaz))
        {
            if (!strcmp(tipo_interfaz, interfaz_solicitada->tipo))
            {
                hacer_peticion(proceso_a_interfaz, interfaz_solicitada);
            }
            else
            {
                log_info(logger_kernel, "Finaliza el proceso %d - INVALID_INTERFACE", proceso_a_interfaz->proceso->pid);
                log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso_a_interfaz->proceso->pid);
                entrar_a_exit(proceso_a_interfaz->proceso);
            }
        }
        else
        {
            log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERFAZ NO CONECTADA", proceso_a_interfaz->proceso->pid);
            log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso_a_interfaz->proceso->pid);
            entrar_a_exit(proceso_a_interfaz->proceso);
        }
    }
    else
    {
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_a_interfaz->proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso_a_interfaz->proceso->pid);
        entrar_a_exit(proceso_a_interfaz->proceso);
    }
}

interfaz_t *buscar_interfaz(char *nombre)
{
    pthread_mutex_lock(&mutex_lista_interfaces);
    interfaz_a_consultar = nombre;
    interfaz_t *interfaz_buscada = list_find(interfaces, (void *)es_la_interfaz);
    pthread_mutex_unlock(&mutex_lista_interfaces);
    return interfaz_buscada;
}

bool es_la_interfaz(interfaz_t *interfaz)
{
    return !strcmp(interfaz_a_consultar, interfaz->nombre);
}
void hacer_io_gen_sleep(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    uint32_t uni_de_trabajo = proceso_interfaz->uni_de_trabajo;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t) + sizeof(uint32_t));
        int offset = 0;
        agregar_opcode(stream, &offset, IO_GEN_SLEEP);
        agregar_uint32_t(stream, &offset, pid_proceso);
        agregar_uint32_t(stream, &offset, uni_de_trabajo);
        int socket_generica = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_generica, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_stdin_read(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    uint32_t cant_paginas = proceso_interfaz->cant_paginas;
    char *direcciones_bytes = proceso_interfaz->direcciones_bytes;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + 3 * sizeof(uint32_t) + string_length(direcciones_bytes) + 1);
        int offset = 0;
        agregar_opcode(stream, &offset, IO_STDIN_READ);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_uint32_t(stream, &offset, cant_paginas);
        agregar_string(stream, &offset, direcciones_bytes);
        int socket_stdin = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_stdin, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_stdout_write(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    uint32_t cant_paginas = proceso_interfaz->cant_paginas;
    char *direcciones_bytes = proceso_interfaz->direcciones_bytes;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + 3 * sizeof(uint32_t) + string_length(direcciones_bytes) + 1);
        int offset = 0;
        agregar_opcode(stream, &offset, IO_STDOUT_WRITE);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_uint32_t(stream, &offset, cant_paginas);
        agregar_string(stream, &offset, direcciones_bytes);
        int socket_stdout = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_stdout, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_fs_create(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    char* nombre_archivo = proceso_interfaz->nombre_archivo;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + 2 * sizeof(uint32_t) + string_length(nombre_archivo) + 1);
        int offset = 0;
        agregar_opcode(stream, &offset, IO_FS_CREATE);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_string(stream, &offset, nombre_archivo);
        int socket_dialfs = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_dialfs, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_fs_delete(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    char* nombre_archivo = proceso_interfaz->nombre_archivo;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + 2 * sizeof(uint32_t) + string_length(nombre_archivo) + 1);
        int offset = 0;
        agregar_opcode(stream, &offset, IO_FS_DELETE);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_string(stream, &offset, nombre_archivo);
        int socket_dialfs = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_dialfs, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
     else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_fs_truncate(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    char* nombre_archivo = proceso_interfaz->nombre_archivo;
    uint32_t registro_tamanio = proceso_interfaz->registro_tamanio;
    uint32_t pid_proceso = proceso->pid;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t)*3 + strlen(nombre_archivo) + 1);
        int offset = 0;
        agregar_opcode(stream, &offset, IO_FS_TRUNCATE);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_string(stream, &offset, nombre_archivo);
        agregar_uint32_t(stream, &offset, registro_tamanio);
        int socket_dialfs = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_dialfs, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_fs_write(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    char* nombre_archivo = proceso_interfaz->nombre_archivo;
    uint32_t registro_puntero = proceso_interfaz->registro_puntero;
    uint32_t cant_paginas = proceso_interfaz->cant_paginas;
    char* direcciones_bytes = proceso_interfaz->direcciones_bytes;
    uint32_t pid_proceso = proceso->pid;
    uint32_t tamanio = proceso_interfaz->registro_tamanio;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t)* 6 + strlen(nombre_archivo) + 2 + strlen(direcciones_bytes));
        int offset = 0;
        agregar_opcode(stream, &offset, IO_FS_WRITE);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_string(stream, &offset, nombre_archivo);
        agregar_uint32_t(stream, &offset, registro_puntero);       
        agregar_uint32_t(stream, &offset, tamanio);
        agregar_uint32_t(stream, &offset, cant_paginas);
        agregar_string(stream, &offset, direcciones_bytes);
        int socket_dialfs = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_dialfs, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void hacer_io_fs_read(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz)
{
    proceso_t *proceso = proceso_interfaz->proceso;
    char* nombre_archivo = proceso_interfaz->nombre_archivo;
    uint32_t registro_puntero = proceso_interfaz->registro_puntero;
    uint32_t cant_paginas = proceso_interfaz->cant_paginas;
    char* direcciones_bytes = proceso_interfaz->direcciones_bytes;
    uint32_t pid_proceso = proceso->pid;
    uint32_t tamanio = proceso_interfaz->registro_tamanio;
    bool _pids_iguales(uint32_t pid)
    {
        return pids_iguales(pid, pid_proceso);
    }
    pthread_mutex_lock(&interfaz->mutex_cola);
    list_add(interfaz->cola, proceso);
    pthread_mutex_unlock(&interfaz->mutex_cola);
    pthread_mutex_lock(&interfaz->mutex_exec);
    if (!list_any_satisfy(pids_eliminados, _pids_iguales))
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        list_get(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t)* 6 + strlen(nombre_archivo) + 2 + strlen(direcciones_bytes));
        int offset = 0;
        agregar_opcode(stream, &offset, IO_FS_READ);
        agregar_uint32_t(stream, &offset, proceso->pid);
        agregar_string(stream, &offset, nombre_archivo);
        agregar_uint32_t(stream, &offset, registro_puntero);
        agregar_uint32_t(stream, &offset, tamanio);
        agregar_uint32_t(stream, &offset, cant_paginas);
        agregar_string(stream, &offset, direcciones_bytes);
        int socket_dialfs = (int)dictionary_get(diccionario_interfaces, interfaz->nombre);
        send(socket_dialfs, stream, offset, 0);
        free(stream);
        sem_wait(&interfaz->sem_vuelta);
        recibir_fin_de_peticion(interfaz);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
    }
}

void volver_a_exec(proceso_t *proceso, uint32_t tiempo_en_cpu, t_temporal *timer)
{
    if (!strcmp("VRR", algoritmo_planificacion))
    {
        proceso->quantum += tiempo_en_cpu;
    }
    void *stream = malloc(sizeof(op_code));
    int offset = 0;
    agregar_opcode(stream, &offset, VUELTA_A_EXEC);
    log_info(logger_kernel, "OPCODE: %d", *((int *)stream));
    send(cpu_dispatch_fd, stream, offset, 0);
    free(stream);
    if (!strcmp("VRR", algoritmo_planificacion) || !strcmp("RR", algoritmo_planificacion))
    {
        esperar_llegada_de_proceso_rr_vrr(proceso, timer, logger_kernel);
    }
    else
    {
        esperar_llegada_de_proceso_fifo(proceso, logger_kernel, timer);
    }
}
/*
void recibir_fin_io_stdin_read(interfaz_t *interfaz)
{
    pthread_mutex_lock(&mutex_stdin_list);
    proceso_t *proceso_a_desbloquear = list_remove(pcbs_stdin, 0);
    pthread_mutex_unlock(&mutex_stdin_list);
    pthread_mutex_unlock(&mutex_stdin_exec);
    volver_a_ready(proceso_a_desbloquear);
}

void recibir_fin_io_stdout_write()
{
    pthread_mutex_lock(&mutex_stdout_list);
    proceso_t *proceso_a_desbloquear = list_remove(pcbs_stdout, 0);
    pthread_mutex_unlock(&mutex_stdout_list);
    pthread_mutex_unlock(&mutex_stdout_exec);
    volver_a_ready(proceso_a_desbloquear);
}
*/
void recibir_fin_de_peticion(interfaz_t *interfaz)
{
    if (interfaz->fin_de_proceso != 1)
    {
        pthread_mutex_lock(&interfaz->mutex_cola);
        proceso_t *proceso_a_desbloquear = list_remove(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        pthread_mutex_unlock(&interfaz->mutex_exec);
        verificar_detencion_de_planificacion();
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <READY>", proceso_a_desbloquear->pid);
        volver_a_ready(proceso_a_desbloquear);
    }
    else
    {
        pthread_mutex_unlock(&interfaz->mutex_exec);
        sem_post(&interfaz->sem_eliminar_proceso);
    }
}
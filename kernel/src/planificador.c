#include <../include/planificador.h>

void planificar_nuevo_proceso(void *void_args)
{
    nuevo_proceso_t *args = (nuevo_proceso_t *)void_args;
    proceso_t *proceso = args->proceso;
    t_log *logger = args->logger;
    free(args);
    ingresar_a_new(proceso);
    ingresar_a_ready();
    ingresar_a_exec();
}

void ingresar_a_new(proceso_t *proceso)
{
    pthread_mutex_lock(&mutex_new_list);
    list_add(pcbs_new, (void *)proceso);
    pthread_mutex_unlock(&mutex_new_list);
    log_info(logger_kernel, "Se crea el proceso <%d> en NEW", proceso->pid);

    // sem_post(&pcb_esperando_ready);
}

void ingresar_a_ready()
{
    // sem_wait(&pcb_esperando_ready);
    sem_wait(&multiprogramacion);

    proceso_t *proceso = obtenerSiguienteAReady();
    if (proceso != NULL)
    {
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <READY>", proceso->pid);
        pthread_mutex_lock(&mutex_ready_list);
        list_add(pcbs_ready, (void *)proceso);
        mostrar_pids_cola(pcbs_ready, "READY");
        pthread_mutex_unlock(&mutex_ready_list);
        // sem_post(&pcb_esperando_exec);
    }
    else
    {
        sem_post(&multiprogramacion);
    }
}

void mostrar_pids_cola(t_list *cola, char *nombre_cola)
{
    t_list *pids = list_map(cola, (void *)_get_pid);
    char *buffer = string_new();
    for (int i = 0; i < list_size(cola); i++)
    {
        char *pid_a_mostrar = string_itoa(list_get(pids, i));
        string_append(&buffer, pid_a_mostrar);
        string_append(&buffer, " ");
        free(pid_a_mostrar);
    }
    log_info(logger_kernel, "Cola <%s>: %s", nombre_cola, buffer);
    free(buffer);
    list_destroy(pids);
}

uint32_t _get_pid(proceso_t *proceso)
{
    return proceso->pid;
}

proceso_t *obtenerSiguienteAReady()
{
    proceso_t *pcb;
    pthread_mutex_lock(&mutex_new_list);
    if (!list_is_empty(pcbs_new))
    {
        pcb = list_remove(pcbs_new, 0);
    }
    else
    {
        pcb = NULL;
    }
    pthread_mutex_unlock(&mutex_new_list);
    return pcb;
}

void ingresar_a_exec()
{
    // sem_wait(&pcb_esperando_exec);
    pthread_mutex_lock(&mutex_exec_list);
    proceso_t *proceso = obtenerSiguienteAExec();
    if (proceso != NULL)
    {
        list_add(pcbs_exec, (void *)proceso);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXEC>", proceso->pid);
        ejecutar_proceso(proceso, logger_kernel, proceso->quantum);
    }
    else
    {
        pthread_mutex_unlock(&mutex_exec_list);
    }
}

proceso_t *obtenerSiguienteAExec()
{
    proceso_t *pcb;
    if (!list_is_empty(pcbs_ready_prioritarios))
    {
        pthread_mutex_lock(&mutex_ready_prioritario_list);
        pcb = list_remove(pcbs_ready_prioritarios, 0);
        pthread_mutex_unlock(&mutex_ready_prioritario_list);
    }
    else if (!list_is_empty(pcbs_ready))
    {
        pthread_mutex_lock(&mutex_ready_list);
        pcb = list_remove(pcbs_ready, 0);
        pthread_mutex_unlock(&mutex_ready_list);
    }
    else
    {
        pcb = NULL;
    }
    return pcb;
}

void liberar_cpu()
{
    list_remove(pcbs_exec, 0);
    pthread_mutex_unlock(&mutex_exec_list);
}

void ejecutar_proceso(proceso_t *proceso, t_log *logger, int unQuantum)
{
    t_temporal *timer = temporal_create();
    if (!strcmp(algoritmo_planificacion, "FIFO"))
    {
        enviar_proceso_a_cpu(proceso, logger);
        esperar_llegada_de_proceso_fifo(proceso, logger, timer);
    }
    else if (!strcmp(algoritmo_planificacion, "RR") || !strcmp(algoritmo_planificacion, "VRR"))
    {
        ejecuciones++;
        enviar_proceso_a_cpu(proceso, logger);
        pthread_t hilo_interrupcion;
        interrupcion_proceso_t *args_interrupcion = malloc(sizeof(interrupcion_proceso_t));
        args_interrupcion->proceso = proceso;
        args_interrupcion->timer = timer;
        args_interrupcion->logger = logger;
        args_interrupcion->ejecucion = ejecuciones;
        pthread_create(&hilo_interrupcion, NULL, (void *)manejar_interrupcion_de_timer, (void *)args_interrupcion);
        esperar_llegada_de_proceso_rr_vrr(proceso, timer, logger);
        pthread_detach(hilo_interrupcion);
    }
}

void enviar_proceso_a_cpu(proceso_t *proceso, t_log *logger)
{
    void *stream = malloc(sizeof(op_code) + 9 * sizeof(uint32_t) + 4 * sizeof(uint8_t));
    int offset = 0;
    agregar_pcb(stream, &offset, proceso);
    send(cpu_dispatch_fd, stream, offset, 0);
    free(stream);
}

void agregar_pcb(void *stream, int *offset, proceso_t *proceso)
{
    agregar_opcode(stream, offset, ENVIAR_PCB);
    agregar_uint32_t(stream, offset, proceso->pid);
    agregar_uint32_t(stream, offset, proceso->quantum);
    agregar_uint32_t(stream, offset, proceso->registros->PC);
    agregar_uint8_t(stream, offset, proceso->registros->AX);
    agregar_uint8_t(stream, offset, proceso->registros->BX);
    agregar_uint8_t(stream, offset, proceso->registros->CX);
    agregar_uint8_t(stream, offset, proceso->registros->DX);
    agregar_uint32_t(stream, offset, proceso->registros->EAX);
    agregar_uint32_t(stream, offset, proceso->registros->EBX);
    agregar_uint32_t(stream, offset, proceso->registros->ECX);
    agregar_uint32_t(stream, offset, proceso->registros->EDX);
    agregar_uint32_t(stream, offset, proceso->registros->SI);
    agregar_uint32_t(stream, offset, proceso->registros->DI);
}

void esperar_llegada_de_proceso_fifo(proceso_t *proceso, t_log *logger, t_temporal *timer)
{
    uint32_t pid;
    uint32_t quantum;
    recv(cpu_dispatch_fd, &pid, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &quantum, sizeof(uint32_t), 0);
    esperar_contexto_de_ejecucion(proceso, logger, timer, 0);
}

bool es_el_proceso(proceso_t *proceso, proceso_t *proceso_a_encontrar)
{
    return proceso_a_encontrar == proceso;
}

void manejar_interrupcion_de_timer(void *args_void)
{
    interrupcion_proceso_t *args = (interrupcion_proceso_t *)args_void;
    t_log *logger = args->logger;
    t_temporal *timer = args->timer;
    proceso_t *proceso = args->proceso;
    int ejecucion = args->ejecucion;
    free(args);
    usleep(proceso->quantum * 1000);
    bool _es_el_proceso(proceso_t * proceso_a_encontrar)
    {
        return es_el_proceso(proceso_a_encontrar, proceso);
    }
    if (ejecucion == ejecuciones && list_find(pcbs_exec, (void *)_es_el_proceso))
    {
        //temporal_stop(timer);
        log_info(logger_kernel, "PID: %d - Desalojado por fin de Quantum", proceso->pid);
        mandar_fin_de_quantum_de(proceso);
    }
}

void mandar_fin_de_quantum_de(proceso_t *proceso)
{
    void *stream = malloc(sizeof(op_code));
    int offset = 0;
    agregar_opcode(stream, &offset, INTERRUMPIR);
    send(cpu_interrupt_fd, stream, offset, 0);
    free(stream);
}

void esperar_llegada_de_proceso_rr_vrr(proceso_t *proceso, t_temporal *timer, t_log *logger)
{
    uint32_t pid;
    uint32_t pquantum;
    recv(cpu_dispatch_fd, &pid, sizeof(uint32_t), 0);
    uint32_t tiempo_en_cpu = (uint32_t)temporal_gettime(timer);
    recv(cpu_dispatch_fd, &pquantum, sizeof(uint32_t), 0);
    if (!strcmp(algoritmo_planificacion, "VRR"))
    {
        if (tiempo_en_cpu < proceso->quantum)
        {
            proceso->quantum -= tiempo_en_cpu;
        }
        else
        {
            proceso->quantum = quantum;
        }
    }
    esperar_contexto_de_ejecucion(proceso, logger, timer, tiempo_en_cpu);
}

void esperar_contexto_de_ejecucion(proceso_t *proceso, t_log *logger, t_temporal *timer, uint32_t tiempo_en_cpu)
{
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
    uint32_t size_motivo;
    char *motivo_de_desalojo;

    recv(cpu_dispatch_fd, &PC, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &AX, sizeof(uint8_t), 0);
    recv(cpu_dispatch_fd, &BX, sizeof(uint8_t), 0);
    recv(cpu_dispatch_fd, &CX, sizeof(uint8_t), 0);
    recv(cpu_dispatch_fd, &DX, sizeof(uint8_t), 0);
    recv(cpu_dispatch_fd, &EAX, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &EBX, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &ECX, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &EDX, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &SI, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &DI, sizeof(uint32_t), 0);
    recv(cpu_dispatch_fd, &size_motivo, sizeof(uint32_t), 0);
    motivo_de_desalojo = malloc(size_motivo);
    recv(cpu_dispatch_fd, motivo_de_desalojo, size_motivo, 0);

    proceso->registros->PC = PC;
    proceso->registros->AX = AX;
    proceso->registros->BX = BX;
    proceso->registros->CX = CX;
    proceso->registros->DX = DX;
    proceso->registros->EAX = EAX;
    proceso->registros->EBX = EBX;
    proceso->registros->ECX = ECX;
    proceso->registros->EDX = EDX;
    proceso->registros->SI = SI;
    proceso->registros->DI = DI;

    char **substrings;
    char *instruccion_de_motivo_string;
    if (motivo_de_desalojo[strlen(motivo_de_desalojo) - 1] == '\n')
    {
        motivo_de_desalojo[strlen(motivo_de_desalojo) - 1] = '\0';
    }
    if (string_contains(motivo_de_desalojo, " "))
    {
        substrings = string_split(motivo_de_desalojo, " ");
        instruccion_de_motivo_string = substrings[0];
    }
    else
    {
        instruccion_de_motivo_string = motivo_de_desalojo;
    }

    op_code instruccion_de_motivo = string_to_opcode(instruccion_de_motivo_string);

    if (instruccion_de_motivo != WAIT && instruccion_de_motivo != SIGNAL)
    {
        temporal_stop(timer);
        temporal_destroy(timer);
        verificar_detencion_de_planificacion();
        liberar_cpu();
    }
    proceso_a_interfaz_t *proceso_interfaz = malloc(sizeof(proceso_a_interfaz_t));
    proceso_interfaz->proceso = proceso;

    switch (instruccion_de_motivo)
    {
    case IO_GEN_SLEEP:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->uni_de_trabajo = atoi(substrings[2]);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "GENERICA", hacer_io_gen_sleep);
        break;
    case IO_STDIN_READ:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->cant_paginas = atoi(substrings[2]);
        proceso_interfaz->direcciones_bytes = substrings[3];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "STDIN", hacer_io_stdin_read);
        break;
    case IO_STDOUT_WRITE:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->cant_paginas = atoi(substrings[2]);
        proceso_interfaz->direcciones_bytes = substrings[3];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "STDOUT", hacer_io_stdout_write);
        break;
    case IO_FS_CREATE:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->nombre_archivo = substrings[2];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "DIALFS", hacer_io_fs_create);
        break;
    case IO_FS_DELETE:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->nombre_archivo = substrings[2];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "DIALFS", hacer_io_fs_delete);
        break;
    case IO_FS_TRUNCATE:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->nombre_archivo = substrings[2];
        proceso_interfaz->registro_tamanio = atoi(substrings[3]);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "DIALFS", hacer_io_fs_truncate);
        break;
    case IO_FS_WRITE:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->nombre_archivo = substrings[2];
        proceso_interfaz->registro_puntero = atoi(substrings[3]);
        proceso_interfaz->registro_tamanio = atoi(substrings[4]);
        proceso_interfaz->cant_paginas = atoi(substrings[5]);
        proceso_interfaz->direcciones_bytes = substrings[6];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "DIALFS", hacer_io_fs_write);
        break;
    case IO_FS_READ:
        proceso_interfaz->interfaz = substrings[1];
        proceso_interfaz->nombre_archivo = substrings[2];
        proceso_interfaz->registro_puntero = atoi(substrings[3]);
        proceso_interfaz->registro_tamanio = atoi(substrings[4]);
        proceso_interfaz->cant_paginas = atoi(substrings[5]);
        proceso_interfaz->direcciones_bytes = substrings[6];
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCKED>", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Bloqueado por: <%s>", proceso->pid, proceso_interfaz->interfaz);
        enviar_proceso_a_interfaz(proceso_interfaz, "DIALFS", hacer_io_fs_read);
        break;
    case WAIT:
        char *recurso_wait = substrings[1];
        enviar_proceso_a_wait(proceso, recurso_wait, tiempo_en_cpu, timer);
        break;
    case SIGNAL:
        char *recurso_signal = substrings[1];
        enviar_proceso_a_signal(proceso, recurso_signal, tiempo_en_cpu, timer);
        break;
    case EXIT:
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: SUCCESS", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
        break;
    case TIMER:
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <READY>", proceso->pid);
        proceso->quantum = quantum;
        pthread_mutex_unlock(&mutex_ready_list);
        list_add(pcbs_ready, proceso);
        mostrar_pids_cola(pcbs_ready, "READY");
        pthread_mutex_unlock(&mutex_ready_list);
        // sem_post(&pcb_esperando_exec);
        ingresar_a_exec();
        break;
    case OUTOFMEMORY:
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
        break;
    case FINALIZAR_PROCESO:
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
        break;
    case RECURSO_INVALIDO:
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
        break;
    default:
    }
    if (string_contains(motivo_de_desalojo, " "))
    {
        string_array_destroy(substrings);
    }
    free(motivo_de_desalojo);

    free(proceso_interfaz);
}

void finalizar_proceso(proceso_t *proceso)
{
    void *stream = malloc(sizeof(op_code) + sizeof(uint32_t));
    int offset = 0;
    agregar_opcode(stream, &offset, FINALIZAR_PROCESO);
    agregar_uint32_t(stream, &offset, proceso->pid);
    send(memoria_interrupt_fd, stream, offset, 0);
    list_add(pids_eliminados, proceso->pid);
    free(stream);
    free(proceso->recursos);
    free(proceso->registros);
    free(proceso);
    proceso = NULL;
}

void ingresar_a_exit(proceso_t *proceso)
{
    pthread_mutex_lock(&mutex_exit_queue);
    queue_push(pcbs_exit, (void *)proceso);
    pthread_mutex_unlock(&mutex_exit_queue);

    // sem_post(&pcb_esperando_exit);
}

void realizar_exit()
{
    // sem_wait(&pcb_esperando_exit);

    pthread_mutex_lock(&mutex_exit_queue);
    proceso_t *proceso = queue_pop(pcbs_exit);
    pthread_mutex_unlock(&mutex_exit_queue);
    finalizar_proceso(proceso);
}

void finalizar_proceso_de_pid(uint32_t pid_proceso)
{
    pid_a_finalizar = pid_proceso;
    buscar_en_cola_y_finalizar_proceso(pcbs_new, mutex_new_list);
    buscar_en_cola_y_finalizar_proceso(pcbs_ready, mutex_ready_list);
    buscar_en_cola_y_finalizar_proceso(pcbs_ready_prioritarios, mutex_ready_prioritario_list);
    int indice = 0;
    while (indice < list_size(interfaces))
    {
        interfaz_t *interfaz = list_get(interfaces, indice);
        buscar_en_cola_de_bloqueados_y_finalizar_proceso(interfaz);
        indice++;
    }
    buscar_en_colas_de_bloqueados_wait_y_finalizar_proceso();
    buscar_en_exec_y_finalizar_proceso();
}

bool tiene_el_pid(proceso_t *proceso)
{
    return pid_a_finalizar == proceso->pid;
}

void buscar_en_cola_y_finalizar_proceso(t_list *cola, pthread_mutex_t mutex)
{
    pthread_mutex_lock(&mutex);
    proceso_t *proceso = list_remove_by_condition(cola, (void *)tiene_el_pid);
    pthread_mutex_unlock(&mutex);
    if (proceso != NULL)
    {
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso->pid);
        if (cola == pcbs_ready || cola == pcbs_ready_prioritarios)
        {
            log_info(logger_kernel, "PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXIT>", proceso->pid);
            verificar_multiprogramacion();
        }
        else
        {
            log_info(logger_kernel, "PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <EXIT>", proceso->pid);
        }
        desalojar_recursos(proceso);
        ingresar_a_exit(proceso);
        realizar_exit();
    }
}

void buscar_en_cola_de_bloqueados_y_finalizar_proceso(interfaz_t *interfaz)
{
    proceso_t *proceso_a_eliminar;
    pthread_mutex_lock(&interfaz->mutex_cola);

    if (list_find(interfaz->cola, (void *)tiene_el_pid) != NULL && tiene_el_pid(list_get(interfaz->cola, 0)))
    {
        proceso_a_eliminar = list_find(interfaz->cola, (void *)tiene_el_pid);
        pthread_mutex_unlock(&interfaz->mutex_cola);
        pthread_mutex_lock(&interfaz->mutex_fin_de_proceso);
        interfaz->fin_de_proceso = 1;
        pthread_mutex_unlock(&interfaz->mutex_fin_de_proceso);
        sem_wait(&interfaz->sem_eliminar_proceso);
        pthread_mutex_lock(&interfaz->mutex_fin_de_proceso);
        interfaz->fin_de_proceso = 0;
        pthread_mutex_unlock(&interfaz->mutex_fin_de_proceso);
        pthread_mutex_lock(&interfaz->mutex_cola);
        proceso_a_eliminar = list_remove(interfaz->cola, 0);
        pthread_mutex_unlock(&interfaz->mutex_cola);
    }
    else
    {
        proceso_a_eliminar = list_remove_by_condition(interfaz->cola, (void *)tiene_el_pid);
        pthread_mutex_unlock(&interfaz->mutex_cola);
    }

    if (proceso_a_eliminar != NULL)
    {
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso_a_eliminar->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso_a_eliminar->pid);
        entrar_a_exit(proceso_a_eliminar);
    }
}

void buscar_en_colas_de_bloqueados_wait_y_finalizar_proceso()
{
    for (int i = 0; i < cantidad_recursos; i++)
    {
        pthread_mutex_lock(&mutex_recursos_list[i]);
        // pthread_mutex_lock(&mutex_exec_list);
        if (list_find(pcbs_recursos[i], (void *)tiene_el_pid) != NULL && list_find(pcbs_exec, (void *)tiene_el_pid) == NULL)
        {
            proceso_t *proceso = list_remove_by_condition(pcbs_recursos[i], (void *)tiene_el_pid);
            pthread_mutex_unlock(&mutex_exec_list);
            pthread_mutex_unlock(&mutex_recursos_list[i]);
            if (proceso != NULL)
            {
                log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso->pid);
                log_info(logger_kernel, "PID: <%d> - Estado Anterior: <BLOCKED> - Estado Actual: <EXIT>", proceso->pid);
                entrar_a_exit(proceso);
            }
        }
        else
        {
            // pthread_mutex_unlock(&mutex_exec_list);
            pthread_mutex_unlock(&mutex_recursos_list[i]);
        }
    }
}

void buscar_en_exec_y_finalizar_proceso()
{
    // pthread_mutex_lock(&mutex_exec_list);
    if (list_any_satisfy(pcbs_exec, (void *)tiene_el_pid))
    {
        void *stream = malloc(sizeof(op_code) + sizeof(uint32_t));
        int offset = 0;
        agregar_opcode(stream, &offset, FINALIZAR_PROCESO);
        agregar_uint32_t(stream, &offset, pid_a_finalizar);
        send(cpu_interrupt_fd, stream, offset, 0);
        free(stream);
    }
    // pthread_mutex_unlock(&mutex_exec_list);
}

void cambiar_grado_de_multiprogramacion(int nuevo_grado_multiprogramacion)
{
    if (nuevo_grado_multiprogramacion > grado_multiprogramacion)
    {
        for (int i = 0; i < nuevo_grado_multiprogramacion - grado_multiprogramacion; i++)
        {
            sem_post(&multiprogramacion);
        }
    }
    else
    {
        pthread_mutex_lock(&mutex_disminuciones);
        disminuciones_multiprogramacion = grado_multiprogramacion - nuevo_grado_multiprogramacion;
        pthread_mutex_unlock(&mutex_disminuciones);
    }
    grado_multiprogramacion = nuevo_grado_multiprogramacion;
}

void entrar_a_exit(proceso_t *proceso)
{
    desalojar_recursos(proceso);
    verificar_multiprogramacion();
    ingresar_a_exit(proceso);
    realizar_exit();
}

void verificar_multiprogramacion()
{
    pthread_mutex_lock(&mutex_disminuciones);
    if (disminuciones_multiprogramacion > 0)
    {
        disminuciones_multiprogramacion--;
        pthread_mutex_unlock(&mutex_disminuciones);
    }
    else
    {
        pthread_mutex_unlock(&mutex_disminuciones);
        sem_post(&multiprogramacion);
    }
}

void desalojar_recursos(proceso_t *proceso)
{
    for (int i = 0; i < cantidad_recursos; i++)
    {
        if (list_find(pcbs_recursos[i], (void *)tiene_el_pid))
        {
            list_remove_element(pcbs_recursos[i], proceso);
        }
        for (int j = proceso->recursos[i]; j > 0; j--)
        {
            pthread_mutex_lock(&mutex_recursos_instancias[i]);
            instancias_recursos[i]++;
            pthread_mutex_unlock(&mutex_recursos_instancias[i]);
            sem_post(&pcb_esperando_recurso[i]);
        }
    }
}

void verificar_detencion_de_planificacion()
{
    pthread_mutex_lock(&mutex_planificacion_activa);
    if (planificacion_activa == 0)
    {
        pthread_mutex_unlock(&mutex_planificacion_activa);
        pthread_mutex_lock(&mutex_reanudar_planificacion);
        reanudar_planificacion++;
        pthread_mutex_unlock(&mutex_reanudar_planificacion);
        sem_wait(&sem_detener_planificacion);
    }
    else
    {
        pthread_mutex_unlock(&mutex_planificacion_activa);
    }
}

void iniciar_planificacion()
{
    for (int i = 0; i < reanudar_planificacion; i++)
    {
        sem_post(&sem_detener_planificacion);
    }
    pthread_mutex_lock(&mutex_reanudar_planificacion);
    reanudar_planificacion = 0;
    pthread_mutex_unlock(&mutex_reanudar_planificacion);
}

void finalizar_procesos_de_interfaz(char* nombre){
    interfaz_t* interfaz = buscar_interfaz(nombre);
    while(!list_is_empty(interfaz->cola)){
        proceso_t* proceso = list_remove(interfaz->cola, 0);
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: DESCONEXION_INTERFAZ", proceso->pid);
        log_info(logger_kernel, "PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>", proceso->pid);
        entrar_a_exit(proceso);
    }

}

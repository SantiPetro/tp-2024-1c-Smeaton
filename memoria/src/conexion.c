#include <../include/conexion.h>

void procesar_conexion(void* args_void) {
    conexion_args_t* args = (conexion_args_t*) args_void;
    int socket_cliente = args->socket_cliente;
    t_log* logger = args->logger;
    free(args);

    op_code opcode;
    while (socket_cliente != 1) {
        if ((recv(socket_cliente, &opcode, sizeof(op_code), MSG_WAITALL)) != sizeof(op_code)){
            //log_info(logger, "Tiro error");
            return;
        }
        
        switch(opcode) {
            case TAMANIOPAGINA:
                void* stream = malloc(sizeof(uint32_t));
                int offset = 0;
                agregar_uint32_t(stream, &offset, tam_pagina);
                send(socket_cliente, stream, offset, 0);
                free(stream);
                break;
            case INICIAR_PROCESO:
                usleep(retardo_respuesta * 1000);                
                uint32_t size;
                recv(socket_cliente, &size, sizeof(uint32_t), 0);
                char* path_con_barra = malloc(size);
                recv(socket_cliente, path_con_barra, size, 0);
                uint32_t pid;
                recv(socket_cliente, &pid, sizeof(uint32_t), 0);
                char* path = string_substring_from(path_con_barra, 1);
                free(path_con_barra);
                if(existe_archivo(path)) {
                    pthread_mutex_lock(&mutex_archivo_proceso);
                    agregar_proceso(archivos_procesos, path, pid);
                    pthread_mutex_unlock(&mutex_archivo_proceso);
                    int cant_marcos = cantidad_marcos();
                    tabla_t* tabla_paginas = iniciar_tabla(pid, cant_marcos);
                    log_info(logger, "PID: <%d> - Tamaño: <0>", pid);
                    pthread_mutex_lock(&mutex_paginas);
                    list_add(tablas_paginas_memoria, tabla_paginas);
                    pthread_mutex_unlock(&mutex_paginas);
                } else {
                    pid = 0;
                }
                enviar_pid(socket_cliente, pid);
                break;
            case FINALIZAR_PROCESO:
                usleep(retardo_respuesta * 1000);
                uint32_t pid_a_finalizar; 
                recv(socket_cliente, &pid_a_finalizar, sizeof(uint32_t), 0);
                pthread_mutex_lock(&mutex_paginas);
                log_info(logger, "PID: <%d> - Tamaño: <%d>", pid_a_finalizar, cantidad_paginas_proceso(pid_a_finalizar));
                reducir_tamanio_proceso(pid_a_finalizar, tamanio_proceso(pid_a_finalizar));
                eliminar_tabla(pid_a_finalizar);
                pthread_mutex_unlock(&mutex_paginas);
                pthread_mutex_lock(&mutex_archivo_proceso);
                eliminar_archivo_proceso(archivos_procesos, pid_a_finalizar);
                pthread_mutex_unlock(&mutex_archivo_proceso);
                break;
            case FETCH:
                usleep(retardo_respuesta * 1000);
                uint32_t pid_a_buscar;
                recv(socket_cliente, &pid_a_buscar, sizeof(uint32_t), 0);
                uint32_t pc;
                recv(socket_cliente, &pc, sizeof(uint32_t), 0);
                pthread_mutex_lock(&mutex_archivo_proceso);
                char* instruccion = buscar_instruccion(pid_a_buscar, pc, archivos_procesos);
                pthread_mutex_unlock(&mutex_archivo_proceso);
                if(instruccion[strlen(instruccion) - 1] == '\n') {
                    instruccion[strlen(instruccion) - 1] = '\0';
                }
                enviar_instruccion(socket_cliente, instruccion);
                free(instruccion);
                break;
            case LEER:
                usleep(retardo_respuesta * 1000);
                uint16_t dir_fis_mov_in;
                uint16_t bytes_mov_in;  
                uint32_t pid_mov_in;
                recv(socket_cliente, &pid_mov_in, sizeof(uint32_t), 0);
                recv(socket_cliente, &dir_fis_mov_in, sizeof(uint16_t), 0);
                recv(socket_cliente, &bytes_mov_in, sizeof(uint16_t), 0);
                void* lectura = malloc(bytes_mov_in);
                leer(lectura, dir_fis_mov_in, bytes_mov_in, logger, pid_mov_in);
                enviar_lectura(socket_cliente, lectura, bytes_mov_in);
                free(lectura);
                break;
            case ESCRIBIR:
                usleep(retardo_respuesta * 1000);
                uint16_t dir_fis_mov_out;
                uint16_t bytes_mov_out;
                uint32_t pid_mov_out;
                recv(socket_cliente, &pid_mov_out, sizeof(uint32_t), 0);
                recv(socket_cliente, &dir_fis_mov_out, sizeof(uint16_t), 0);
                recv(socket_cliente, &bytes_mov_out, sizeof(uint16_t), 0);
                void* valor_mov_out = malloc(bytes_mov_out);
                recv(socket_cliente, valor_mov_out, bytes_mov_out, 0);
                escribir(dir_fis_mov_out, valor_mov_out, bytes_mov_out, logger, pid_mov_out);
                enviar_ok(socket_cliente);
                free(valor_mov_out);
                break;
            case RESIZE:
                usleep(retardo_respuesta * 1000);
                uint32_t tamanio;
                recv(socket_cliente, &tamanio, sizeof(uint32_t), 0);
                uint32_t pid_resize;
                recv(socket_cliente, &pid_resize, sizeof(uint32_t), 0);
                log_info(logger, "Pid invalid: %d\n ", pid_resize);
                pthread_mutex_lock(&mutex_paginas);
                int tam_proceso = tamanio_proceso(pid_resize);
                log_info(logger, "Tamanio: %d\n ", tam_proceso);
                int response = 0;
                if(tamanio >= tam_proceso) {
                    log_info(logger, "PID: <%d> - Tamaño Actual: <%d> - Tamaño a Ampliar: <%d>", pid_resize, tam_proceso, tamanio - tam_proceso);
                    response = ampliar_tamanio_proceso(pid_resize, tamanio-tam_proceso);
                    pthread_mutex_unlock(&mutex_paginas);
                } else {
                    log_info(logger, "PID: <%d> - Tamaño Actual: <%d> - Tamaño a Reducir: <%d>", pid_resize, tam_proceso, tam_proceso - tamanio);
                    reducir_tamanio_proceso(pid_resize, tam_proceso - tamanio);
                    pthread_mutex_unlock(&mutex_paginas);
                }
                if(response) {
                    enviar_out_of_memory(socket_cliente);
                } else {
                    enviar_ok(socket_cliente);
                }
                break;
            case PEDIR_MARCO:
                usleep(retardo_respuesta * 1000);
                uint32_t pid_marco;
                uint32_t nro_pagina;
                recv(socket_cliente, &pid_marco, sizeof(uint32_t), 0);
                recv(socket_cliente, &nro_pagina, sizeof(uint32_t), 0);
                pthread_mutex_lock(&mutex_paginas);
                tabla_t* tabla = tabla_paginas_por_pid(pid_marco);
                pthread_mutex_unlock(&mutex_paginas);
                pagina_t* pagina = buscar_pagina_por_nro(tabla, (int)nro_pagina);
                log_info(logger, "PID: <%d> - Pagina: <%d> - Marco: <%d>", pid_marco, nro_pagina, pagina->marco);
                void* stream_marco = malloc(sizeof(uint16_t));
                int offset_marco = 0;
                agregar_uint16_t(stream_marco, &offset_marco, pagina->marco);
                send(socket_cliente, stream_marco, offset_marco, 0);
                free(stream_marco);
                break;
            default:
                break;
        }
    }
    
    return;
}

void enviar_pid(int socket_cliente, uint32_t pid) {
    void* stream = malloc(sizeof(uint32_t));
    int offset = 0;
    agregar_uint32_t(stream, &offset, pid);
    send(socket_cliente, stream, sizeof(uint32_t), 0);
    free(stream);
}

void enviar_instruccion(int socket, char* instruccion) {
    void* stream = malloc(sizeof(uint32_t) + string_length(instruccion) + 1);
    int offset = 0;
    agregar_string(stream, &offset, instruccion);
    send(socket, stream, offset, 0);
    free(stream);
}

void enviar_out_of_memory(int socket_cliente) {
    void* stream = malloc(sizeof(op_code));
    int offset = 0;
    agregar_opcode(stream, &offset, OUTOFMEMORY);
    send(socket_cliente, stream, offset, 0);
    free(stream);
}


void enviar_ok(int socket_cliente) {
    void* stream = malloc(sizeof(op_code));
    int offset = 0;
    agregar_opcode(stream, &offset, MSG);
    send(socket_cliente, stream, offset, 0);
    free(stream);
}

void enviar_lectura(int socket_cliente, void* lectura, uint16_t cant_bytes) {
    void* stream = malloc(cant_bytes + sizeof(uint16_t));
    int offset = 0;
    agregar_uint16_t(stream, &offset, cant_bytes);
    memcpy(stream + offset, lectura, cant_bytes);
    offset += cant_bytes;
    send(socket_cliente, stream, offset, 0);
    free(stream);
}
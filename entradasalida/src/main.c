#include <stdlib.h>
#include <stdio.h>
#include <../include/init.h>
#include <../include/main.h>

void get_config(t_config* config)
 {
    tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_kernel = config_get_string_value(config, "IP_KERNEL"); 
    if(!strcmp(tipo_interfaz, "GENERICA")) {
        tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");    
    }
    else if(!strcmp(tipo_interfaz, "STDIN") || !strcmp(tipo_interfaz, "STDOUT")) {
        puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
        ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    }
    else if(!strcmp(tipo_interfaz, "DIALFS")) {
        tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
        puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
        ip_memoria = config_get_string_value(config, "IP_MEMORIA");
        path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
        block_size = config_get_int_value(config, "BLOCK_SIZE");
        block_count = config_get_int_value(config, "BLOCK_COUNT");
        retraso_compactacion= config_get_int_value(config, "RETRASO_COMPACTACION");
    }
}

int main(int argc, char* argv[]) {
    logger_io = iniciar_logger("io.log", "I/O: ");
    log_info(logger_io, "%s", argv[2]); // ./bin/entradasalida "nombre" "unpath"
    nombre = argv[1];
    //nombre = "FS";
    log_info(logger_io, "%s", nombre); 
    t_config* config_io = iniciar_config(argv[2]);
    //t_config* config_io = iniciar_config("dialfsfs.config");
    get_config(config_io);

    controlar_seniales(logger_io);
    // Se conecta al kernel
    kernel_fd = generar_conexion(logger_io, "kernel", ip_kernel, puerto_kernel, config_io);

    //Se conecta a la memoria
    if(!strcmp(tipo_interfaz, "DIALFS") || !strcmp(tipo_interfaz, "STDIN") || !strcmp(tipo_interfaz, "STDOUT")) 
    {
        memoria_fd = generar_conexion(logger_io, "memoria", ip_memoria, puerto_memoria, config_io);
    }

    conectar_a_kernel(nombre);
    atender_pedidos_kernel();

    terminar_programa(logger_io, config_io);
    liberar_conexion(memoria_fd);
    liberar_conexion(kernel_fd);
    return 0;
}

void conectar_a_kernel(char* nombre) {
  void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + string_length(nombre) + 1);
  int offset = 0;
  agregar_opcode(stream, &offset, string_to_opcode(tipo_interfaz));
  agregar_string(stream, &offset, nombre);
  send(kernel_fd, stream, offset, 0);  
  free(stream);
}

void atender_pedidos_kernel() {

   if(!strcmp("GENERICA", tipo_interfaz)) {
    generica_atender_kernel();
   } 
   if(!strcmp("STDIN", tipo_interfaz)) {
    stdin_atender_kernel();
   } 
   if(!strcmp("STDOUT", tipo_interfaz)) {
    stdout_atender_kernel();
   } 
   if(!strcmp("DIALFS", tipo_interfaz)) {
    iniciar_fs();
    dialfs_atender_kernel();
   } 
}

void generica_atender_kernel() {
    while(1) {
        op_code opcode;
        recv(kernel_fd, &opcode, sizeof(op_code), 0);
        switch(opcode) {
            case IO_GEN_SLEEP:
            uint32_t pid;
            recv(kernel_fd, &pid, sizeof(uint32_t), 0);
            uint32_t unis_de_trabajo;
            recv(kernel_fd, &unis_de_trabajo, sizeof(uint32_t), 0);
            log_info(logger_io, "PID: <%d> - Operacion: <IO_GEN_SLEEP %d>", pid, unis_de_trabajo);
            usleep(tiempo_unidad_trabajo * unis_de_trabajo * 1000);
            fin_de(FIN_DE_SLEEP);
                break;
            default:
        }
    }
}

void stdin_atender_kernel() {
    while(1) {
        op_code opcode;
        recv(kernel_fd, &opcode, sizeof(op_code), 0);
        switch(opcode) {
            case IO_STDIN_READ:
                uint32_t proceso_pid;
                uint32_t cant_paginas;
                uint32_t tamanio_string;
                recv(kernel_fd, &proceso_pid, sizeof(uint32_t), 0);
                recv(kernel_fd, &cant_paginas, sizeof(uint32_t), 0);
                recv(kernel_fd, &tamanio_string, sizeof(uint32_t), 0);
                char* direcciones_bytes = malloc(tamanio_string);
                recv(kernel_fd, direcciones_bytes, tamanio_string, 0);
                log_info(logger_io, "PID: <%d> - Operacion: <IO_STDIN_READ>", proceso_pid);
                char* leido = readline("> ");
                enviar_pedido_stdin(proceso_pid, cant_paginas, direcciones_bytes, leido);
                for(int i=0; i < cant_paginas; i++) {
                    op_code op_code_recep;
                    recv(memoria_fd, &op_code_recep, sizeof(op_code), 0);
                }
                fin_de(FIN_DE_STDIN);
                break;
            default:
        }
    }
}

void stdout_atender_kernel() {
  while(1) {
        op_code opcode;
        recv(kernel_fd, &opcode, sizeof(op_code), 0);
        switch(opcode) {
            case IO_STDOUT_WRITE:
                uint32_t proceso_pid;
                uint32_t cant_paginas;
                uint32_t tamanio_string;
                recv(kernel_fd, &proceso_pid, sizeof(uint32_t), 0);
                log_info(logger_io, "PID: <%d> - Operacion: <IO_STDIN_WRITE>", proceso_pid);
                recv(kernel_fd, &cant_paginas, sizeof(uint32_t), 0);
                recv(kernel_fd, &tamanio_string, sizeof(uint32_t), 0);
                char* direcciones_bytes = malloc(tamanio_string);
                recv(kernel_fd, direcciones_bytes, tamanio_string, 0);
                enviar_lectura_memoria(proceso_pid, cant_paginas, direcciones_bytes);
                char* resultado = string_new();
                recibir_lectura_memoria(cant_paginas, &resultado);
                log_info(logger_io, "%s", resultado);
                free(resultado);
                fin_de(FIN_DE_STDOUT);
                break;
            default:
        }
    }  
}

void dialfs_atender_kernel() {
     while(1) {
        uint32_t tamanio = 0;
        op_code opcode;
        recv(kernel_fd, &opcode, sizeof(op_code), 0);
        uint32_t proceso_pid;
        recv(kernel_fd, &proceso_pid, sizeof(uint32_t), 0);
        uint32_t tam_archivo;
        recv(kernel_fd, &tam_archivo, sizeof(uint32_t), 0);
        char* nombre_arch = malloc(tam_archivo);
        recv(kernel_fd, nombre_arch, tam_archivo, 0);
        usleep(tiempo_unidad_trabajo*1000);
        switch(opcode) {
            case IO_FS_CREATE:
                log_info(logger_io, "PID: <%d> - Operacion: <IO_FS_CREATE>", proceso_pid);
                log_info(logger_io, "PID: <%d> - Crear Archivo: <%s>", proceso_pid, nombre_arch);
                crear_archivo(nombre_arch);
                break;
            case IO_FS_DELETE:
                log_info(logger_io, "PID: <%d> - Operacion: <IO_FS_DELETE>", proceso_pid);
                log_info(logger_io, "PID: <%d> - Eliminar Archivo: <%s>", proceso_pid, nombre_arch);
                eliminar_archivo(nombre_arch, proceso_pid);
                break;
            case IO_FS_TRUNCATE:
                log_info(logger_io, "PID: <%d> - Operacion: <IO_FS_TRUNCATE>", proceso_pid);
                recv(kernel_fd, &tamanio, sizeof(uint32_t), 0);
                log_info(logger_io, "PID: <%d> - Truncar Archivo: <%s> - Tamaño: <%d>", proceso_pid, nombre_arch, tamanio);
                truncar_archivo(nombre_arch, tamanio, proceso_pid);
                break;
            case IO_FS_READ:
                log_info(logger_io, "PID: <%d> - Operacion: <IO_FS_READ>", proceso_pid);
                uint32_t puntero_r;
                recv(kernel_fd, &puntero_r, sizeof(uint32_t), 0);
                recv(kernel_fd, &tamanio, sizeof(uint32_t), 0);
                log_info(logger_io, "PID: <%d> - Leer Archivo: <%s> - Tamaño a Leer: <%d> - Puntero Archivo: <%u>", proceso_pid, nombre_arch, tamanio, puntero_r);
                uint32_t cant_paginas_r;
                recv(kernel_fd, &cant_paginas_r, sizeof(uint32_t), 0);
                uint32_t tam_archivo_dir_bytes_r;
                recv(kernel_fd, &tam_archivo_dir_bytes_r, sizeof(uint32_t), 0);
                char* dir_bytes_r = malloc(tam_archivo_dir_bytes_r);
                recv(kernel_fd, dir_bytes_r, tam_archivo_dir_bytes_r, 0);
                leer_archivo(nombre_arch, puntero_r, cant_paginas_r, dir_bytes_r, proceso_pid);
                break;
            case IO_FS_WRITE:
                log_info(logger_io, "PID: <%d> - Operacion: <IO_FS_WRITE>", proceso_pid);
                uint32_t puntero_w;
                recv(kernel_fd, &puntero_w, sizeof(uint32_t), 0);
                recv(kernel_fd, &tamanio, sizeof(uint32_t), 0);
                log_info(logger_io, "PID: <%d> - Escribir Archivo: <%s> - Tamaño a Escribir: <%d> - Puntero Archivo: <%d>", proceso_pid, nombre_arch, tamanio, puntero_w);
                uint32_t cant_paginas_w;
                recv(kernel_fd, &cant_paginas_w, sizeof(uint32_t), 0);
                uint32_t tam_archivo_dir_bytes_w;
                recv(kernel_fd, &tam_archivo_dir_bytes_w, sizeof(uint32_t), 0);
                char* dir_bytes_w = malloc(tam_archivo_dir_bytes_w);
                recv(kernel_fd, dir_bytes_w, tam_archivo_dir_bytes_w, 0);
                escribir_archivo(nombre_arch, puntero_w, cant_paginas_w, dir_bytes_w, proceso_pid);
                break;
            default:
                break;
        }   
        free(nombre_arch);
        fin_de(FIN_DE_DIALFS);
    }
}

void fin_de(op_code opcode) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + string_length(nombre) + 1);
    int offset = 0;
    agregar_opcode(stream, &offset, opcode);
    agregar_string(stream, &offset, nombre);
    send(kernel_fd, stream, offset, 0);
    free(stream);
}

void enviar_lectura_memoria(uint32_t proceso_pid, uint32_t cant_paginas, char* direcciones_bytes) {
    char** substrings = string_split(direcciones_bytes, "-");
    uint16_t direccion;
    uint16_t bytes;
    for(int i = 0; i < cant_paginas * 2; i+=2) {
        void* stream = malloc(sizeof(op_code) + 3 * sizeof(uint32_t));
        int offset = 0;
        agregar_opcode(stream, &offset, LEER);
        agregar_uint32_t(stream, &offset, proceso_pid);
        direccion = atoi(substrings[i]); 
        bytes = atoi(substrings[i+1]);
        agregar_uint16_t(stream, &offset, direccion); 
        agregar_uint16_t(stream, &offset, bytes);  
        send(memoria_fd, stream, offset, 0);
        free(stream);
     }   
    free(direcciones_bytes);
    string_array_destroy(substrings);
}

void recibir_lectura_memoria(int cant_paginas, char** resultado) {
    for(int i = 0; i < cant_paginas; i++) {
        uint16_t tam;
        char* string_recv;
        recv(memoria_fd, &tam, sizeof(uint16_t), 0);
        string_recv = malloc(tam + 1);
        recv(memoria_fd, string_recv, tam, 0);
        string_recv[tam] = '\0';
        string_append(resultado, string_recv);  
        free(string_recv);
    }
}

void enviar_pedido_stdin(uint32_t proceso_pid, uint32_t cant_paginas, char* direcciones_bytes, char* leido) {
    char** substrings = string_split(direcciones_bytes, "-");
    uint16_t direccion;
    uint32_t bytes;
    int offset_leido = 0;
    char* valor_a_enviar;
    for(int i = 0; i < cant_paginas * 2; i+=2) {
        direccion = atoi(substrings[i]); 
        bytes = atoi(substrings[i+1]);
        void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + 2 * sizeof(uint16_t) + bytes);
        int offset = 0;
        agregar_opcode(stream, &offset, ESCRIBIR); // ESCRIBIR en vez de IO_STDIN_READ
        agregar_uint32_t(stream, &offset, proceso_pid);
        agregar_uint16_t(stream, &offset, direccion);
        valor_a_enviar = malloc(bytes + 1);
        memcpy(valor_a_enviar, leido + offset_leido, bytes);
        valor_a_enviar[bytes] = '\0';
        agregar_string_sin_barra0(stream, &offset, valor_a_enviar);
        send(memoria_fd, stream, offset, 0);
        free(stream);
        offset_leido+= bytes;
        free(valor_a_enviar);
    }
    free(leido);
    free(direcciones_bytes);
    string_array_destroy(substrings);
}

void iniciar_fs() {
    iniciar_bloques();
    iniciar_bitmap();
}

void iniciar_bloques() {
    char* path = string_new();
    string_append(&path, path_base_dialfs);
    string_append(&path, "/bloques.dat");
    f_bloques = fopen(path, "r+b"); 
    if(!f_bloques) {
        f_bloques = fopen(path, "w+b");
        int fd = fileno(f_bloques);
        ftruncate(fd, block_size*block_count);
    }
    fclose(f_bloques);
    free(path);
}

void iniciar_bitmap() {
    char* path = string_new();
    string_append(&path, path_base_dialfs);
    string_append(&path, "/bitmap.dat");
    f_bitmap = fopen(path, "r+b"); 
    if(!f_bitmap) {
        f_bitmap = fopen(path, "w+b");
        int fd = fileno(f_bitmap);
        ftruncate(fd, (block_count+7)/8);
        char *zero_buffer = calloc((block_count+7)/8, sizeof(char));
        fwrite(zero_buffer, sizeof(char), (block_count+7)/8, f_bitmap);
        free(zero_buffer);
    }
    fclose(f_bitmap);
    free(path);
}

void crear_archivo(char* nombre) {
    char* path = string_new();
    path_para_archivo(&path, nombre);
    FILE* f_metadata = fopen(path, "r+b");
    if (!f_metadata) {   
        f_metadata = fopen(path, "w+");
        int bloque_libre = buscar_y_setear_bloque_libre();
        char* bloque_libre_string = string_itoa(bloque_libre);
        if(bloque_libre != -1) { 
            char* metadata_contenido = string_new();
            string_append(&metadata_contenido, "BLOQUE_INICIAL=");
            string_append(&metadata_contenido, bloque_libre_string);
            string_append(&metadata_contenido, "\n");
            string_append(&metadata_contenido, "TAMANIO_ARCHIVO=0");
            fwrite(metadata_contenido, sizeof(char), strlen(metadata_contenido), f_metadata);
            free(bloque_libre_string);
            free(metadata_contenido);
        }
    }
    fclose(f_metadata);
    free(path);
}

void truncar_archivo(char* nombre, uint32_t tamanio_nuevo, uint32_t pid) {
    int tamanio = valor_metadata(nombre, "TAMANIO_ARCHIVO");
    int bloque_inicial = valor_metadata(nombre, "BLOQUE_INICIAL");
    if(tamanio == tamanio_nuevo) {}
    if(tamanio > tamanio_nuevo) {
        cambiar_tamanio_archivo(nombre, tamanio_nuevo, tamanio, bloque_inicial, 0, pid); //Achicar
    }
    if(tamanio < tamanio_nuevo) {
        cambiar_tamanio_archivo(nombre, tamanio_nuevo, tamanio, bloque_inicial, 1, pid); //Agrandar
    }
}

int valor_metadata(char* nombre, char* clave) {
    char* path = string_new();
    path_para_archivo(&path, nombre);
    t_config* config_metadata = config_create(path);
    int bloque_inicial = config_get_int_value(config_metadata, clave);
    config_destroy(config_metadata);
    free(path);
    return bloque_inicial;
}

void path_para_archivo(char** path, char* nombre) {
    string_append(path, path_base_dialfs);
    string_append(path, "/");
    string_append(path, nombre);
}

void cambiar_tamanio_archivo(char* nombre, int tamanio_nuevo, int tamanio, int bloque_inicial, int agrandar, uint32_t pid) {
    int primer_bloque;
    int ultimo_bloque;
    int cant_bloques_actuales = cant_bloques_archivo(tamanio);
    int cant_bloques_restantes = cant_bloques_archivo(tamanio_nuevo);
    if (!agrandar) {
        primer_bloque = cant_bloques_restantes + bloque_inicial;
        ultimo_bloque = cant_bloques_actuales + bloque_inicial - 1;
    }
    if (agrandar) {
        int cant_bloques_nuevos = cant_bloques_restantes - cant_bloques_actuales;
        if (!puede_agrandar_sin_compactar(nombre, tamanio_nuevo, tamanio, bloque_inicial)) {
            log_info(logger_io, "PID: <%d> - Inicio Compactación.", pid);
            bloque_inicial = compactar(nombre);
            usleep(retraso_compactacion*1000);
            log_info(logger_io, "PID: <%d> - Fin Compactación.", pid);
        }
        primer_bloque = cant_bloques_actuales + bloque_inicial;
        ultimo_bloque = cant_bloques_nuevos + cant_bloques_actuales + bloque_inicial -1;
    }
    cambiar_metadata(nombre, tamanio_nuevo, "TAMANIO_ARCHIVO");
    t_bitarray* bitarray = setear_bitarray();
    for (int i = primer_bloque; i <= ultimo_bloque; i++) {
        if(agrandar) {
            bitarray_set_bit(bitarray, i);
        } else {
            bitarray_clean_bit(bitarray, i);
        }
    }
    fseek(f_bitmap, 0, SEEK_SET);
    fwrite(bitarray->bitarray, sizeof(char), (block_count+7)/8, f_bitmap);
    free(bitarray->bitarray);
    fclose(f_bitmap);
    bitarray_destroy(bitarray);  
}

int cant_bloques_archivo(int tamanio) {
    if(!tamanio) return 1;
    if(tamanio == -1) return 0;
    return (tamanio + block_size - 1) / block_size;
}

void cambiar_metadata(char* nombre, int bloque_inicial, char* clave) {
    char* path = string_new();
    path_para_archivo(&path, nombre);
    char* bloque_string = string_itoa(bloque_inicial);

    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    char *file_content = malloc(file_size + 1);
    file_content[file_size] = '\0';
    rewind(file);
    fread(file_content, 1, file_size, file);
    fclose(file);
    char *key_position = strstr(file_content, clave);
    char *value_position = strchr(key_position, '=') + 1;
    char *end_of_line = strchr(value_position, '\n');
    if (end_of_line == NULL) {
        end_of_line = file_content + strlen(file_content);
    }
    size_t new_length = strlen(file_content) + strlen(bloque_string) - (end_of_line - value_position);
    char *new_content = (char *)malloc(new_length + 1);
    size_t prefix_length = value_position - file_content;
    strncpy(new_content, file_content, prefix_length);
    new_content[prefix_length] = '\0';
    strcat(new_content, bloque_string);
    strcat(new_content, end_of_line);
    free(file_content);
    file_content = new_content;
    file = fopen(path, "w");
    fwrite(file_content, 1, strlen(file_content), file);
    fclose(file);
    free(file_content);
    free(path);
    free(bloque_string);
}

bool puede_agrandar_sin_compactar(char* nombre, int tamanio_nuevo, int tamanio, int bloque_inicial) {
    int cant_bloques_nuevos = cant_bloques_archivo(tamanio_nuevo);
    int cant_bloques_actuales = cant_bloques_archivo(tamanio);
    int bloques_a_verificar = cant_bloques_nuevos - cant_bloques_actuales;
    int primer_bloque_a_verificar = bloque_inicial + cant_bloques_actuales;
    t_bitarray* bitarray = setear_bitarray();
    fclose(f_bitmap);
    for (int i = primer_bloque_a_verificar; i < primer_bloque_a_verificar + bloques_a_verificar; i++) {
        if (bitarray_test_bit(bitarray, i)) {
            free(bitarray->bitarray);
            bitarray_destroy(bitarray);
            return false;
        }
    }
    free(bitarray->bitarray);
    bitarray_destroy(bitarray);
    return true;   
}

t_bitarray* setear_bitarray() {
    char* path = string_new();
    path_para_archivo(&path, "bitmap.dat");
    f_bitmap = fopen(path, "r+b");
    size_t tamanio_archivo = (block_count+7)/8;
    char* lectura_f_bloques = malloc(tamanio_archivo);
    fseek(f_bitmap, 0, SEEK_SET);
    fread(lectura_f_bloques, tamanio_archivo, sizeof(char), f_bitmap);
    t_bitarray* bitarray = bitarray_create(lectura_f_bloques,tamanio_archivo);
    free(path);
    return bitarray;
}

int compactar(char* nombre) {
    t_list* bloques_archivos = list_create();
    leer_txt_y_agregar_a_lista(bloques_archivos);
    
    bool mismo_nombre(bloques_archivo_t* bloques_archivo) {
        return !strcmp(bloques_archivo->nombre, nombre);
    }

    bloques_archivo_t* archivo_a_agrandar = list_remove_by_condition(bloques_archivos, (void *)mismo_nombre);
    list_add(bloques_archivos, archivo_a_agrandar);
    void* bloques = leer_bloques_dat();
    void* disco_aux = malloc(block_size*block_count);
    memset(disco_aux, 0, block_size * block_count);
    int offset_escritura = 0;
    int bloque_inicial;
    for (int i = 0; i < list_size(bloques_archivos); i++) {
        bloques_archivo_t* bloques_archivo = list_get(bloques_archivos, i);
        int comienzo_lectura = bloques_archivo->bloque_inicial*block_size;
        memcpy(disco_aux + offset_escritura, bloques + comienzo_lectura, bloques_archivo->tamanio);
        cambiar_metadata(bloques_archivo->nombre, offset_escritura/block_size, "BLOQUE_INICIAL");
        bloque_inicial = offset_escritura/block_size;
        offset_escritura += cant_bloques_archivo(bloques_archivo->tamanio)*block_size;
    }
    free(bloques);
    int bloque_final = bloque_inicial + cant_bloques_archivo(archivo_a_agrandar->tamanio) - 1;
    escribir_bloques_dat(disco_aux);
    compactar_bitarray(bloque_final);
    free(disco_aux);
    list_destroy_and_destroy_elements(bloques_archivos, (void*)bloques_archivo_destroyer);
    return bloque_inicial;
}

void compactar_bitarray(int ultimo_bloque) {
    int tamanio = (block_count+7)/8;
    char* ceros = malloc(tamanio);
    memset(ceros, 0, tamanio);
    t_bitarray* bitarray = bitarray_create(ceros, tamanio);
    for (int i = 0; i <= ultimo_bloque; i++) {
        bitarray_set_bit(bitarray, i);
    }
    char* path = string_new();
    path_para_archivo(&path, "bitmap.dat");
    f_bitmap = fopen(path, "r+b");
    fseek(f_bitmap, 0, SEEK_SET);
    fwrite(bitarray->bitarray, sizeof(char), tamanio, f_bitmap);
    free(ceros);
    free(path);
    bitarray_destroy(bitarray);
    fclose(f_bitmap);
}

void bloques_archivo_destroyer(bloques_archivo_t* bloques_archivo) {
    free(bloques_archivo->nombre);
    free(bloques_archivo);
}

void leer_txt_y_agregar_a_lista(t_list* bloques_archivos) {
    struct dirent *dir;
    DIR *dp = opendir(path_base_dialfs);
    while ((dir = readdir(dp))) {
        if (dir->d_type == DT_REG) {
            size_t len = strlen(dir->d_name);
            if (len > 4 && strcmp(dir->d_name + len - 4, ".txt") == 0) {
                char* nombre = string_new();
                string_append(&nombre, dir->d_name);
                bloques_archivo_t* bloques_archivo = malloc(sizeof(bloques_archivo_t));
                char* path = string_new();
                path_para_archivo(&path, dir->d_name);
                t_config* config_metadata = config_create(path);
                int bloque_inicial = config_get_int_value(config_metadata, "BLOQUE_INICIAL");
                int tamanio = config_get_int_value(config_metadata, "TAMANIO_ARCHIVO");
                bloques_archivo->tamanio = tamanio;
                bloques_archivo->bloque_inicial = bloque_inicial;
                bloques_archivo->nombre = nombre;
                list_add(bloques_archivos, bloques_archivo);
                config_destroy(config_metadata);
                free(path);
            }
        }
    }
    closedir(dp);
}

void* leer_bloques_dat() {
    char* path = string_new();
    path_para_archivo(&path, "bloques.dat");
    f_bloques = fopen(path, "r+b");
    size_t tamanio_archivo = block_count*block_size;
    void* lectura_f_bloques = malloc(tamanio_archivo);
    fseek(f_bloques, 0, SEEK_SET);
    fread(lectura_f_bloques, sizeof(char), tamanio_archivo, f_bloques);
    fclose(f_bloques);
    free(path);
    return lectura_f_bloques;
}

void escribir_bloques_dat(void* bloques) {
    char* path = string_new();
    path_para_archivo(&path, "bloques.dat");
    f_bloques = fopen(path, "r+b");
    fseek(f_bloques, 0, SEEK_SET);
    fwrite(bloques, sizeof(char), block_count*block_size, f_bloques);
    fclose(f_bloques);
    free(path);
}

void eliminar_archivo(char* nombre, uint32_t pid) {
    cambiar_tamanio_archivo(nombre, -1, valor_metadata(nombre, "TAMANIO_ARCHIVO"), valor_metadata(nombre, "BLOQUE_INICIAL"), 0, pid);
    char* path = string_new();
    path_para_archivo(&path, nombre);
    remove(path);
    free(path);
}

void escribir_archivo(char* nombre_arch, uint32_t puntero, uint32_t cant_paginas, char* direcciones_bytes, uint32_t pid) {
    char* string_a_escribir = string_new();
    enviar_lectura_memoria(pid, cant_paginas, direcciones_bytes);
    recibir_lectura_memoria(cant_paginas, &string_a_escribir);
    int bloque_inicial = valor_metadata(nombre_arch, "BLOQUE_INICIAL");
    void* bloques = leer_bloques_dat();
    memcpy(bloques + (bloque_inicial*block_size) + puntero, string_a_escribir, strlen(string_a_escribir));
    escribir_bloques_dat(bloques);
    free(bloques);
    free(string_a_escribir);
}

void leer_archivo(char* nombre_arch, uint32_t puntero, int cant_paginas, char* direcciones_bytes, uint32_t pid) {
    int bloque_inicial = valor_metadata(nombre_arch, "BLOQUE_INICIAL");
    void* bloques = leer_bloques_dat();
    char** substrings = string_split(direcciones_bytes, "-");
    for (int i = 0; i < cant_paginas*2; i=i+2) {
        uint32_t tamanio = (uint32_t)atoi(substrings[i+1]);
        char* parte_a_escribir = malloc(tamanio + 1);
        memcpy(parte_a_escribir, bloques + (bloque_inicial*block_size) + puntero, tamanio);
        parte_a_escribir[tamanio] = '\0';
        uint32_t direccion = (uint32_t)atoi(substrings[i]);
        escribir_memoria(direccion, tamanio, parte_a_escribir, pid);
        puntero += tamanio;
        free(parte_a_escribir);
    }
    free(direcciones_bytes);
    string_array_destroy(substrings);
    free(bloques);
}

void escribir_memoria(uint32_t direccion, uint32_t tamanio, char* parte_a_escribir, uint32_t pid) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + 2 * sizeof(uint16_t) + tamanio);
    int offset = 0;
    agregar_opcode(stream, &offset, ESCRIBIR);
    agregar_uint32_t(stream, &offset, pid);
    agregar_uint16_t(stream, &offset, direccion);
    agregar_string_sin_barra0(stream, &offset, parte_a_escribir);
    send(memoria_fd, stream, offset, 0);
    free(stream);
}
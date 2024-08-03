#include <../include/archivo_proceso.h>
#include <stdbool.h>
#include <stddef.h>

uint32_t pid_a_buscar;

bool existe_archivo(char* path) {
    return fopen(path, "r") != NULL;
}

void agregar_proceso(t_list* archivos_procesos, char* path, uint32_t pid) {
    archivo_proceso_t* archivo_proceso = malloc(sizeof(archivo_proceso_t));
    archivo_proceso->path = path;
    archivo_proceso->pid = pid;
    list_add(archivos_procesos, archivo_proceso);
}


char* buscar_instruccion(uint32_t pid, uint32_t pc, t_list* archivos_procesos) {
    archivo_proceso_t* archivo_proceso_encontrado = archivo_proceso_por_pid(pid, archivos_procesos);
    
    FILE* f = fopen(archivo_proceso_encontrado->path, "r");
    
    fseek(f, 0, SEEK_SET);
    char* instruccion = buscar_instruccion_en(f, pc);
    fclose(f);
    return instruccion;
}

archivo_proceso_t* archivo_proceso_por_pid(uint32_t pid, t_list* archivos_procesos){
    
    bool buscar_por_pid(archivo_proceso_t *archivo_proceso) {
        return (archivo_proceso->pid == pid);
    }
    return list_find(archivos_procesos, (void*)buscar_por_pid);
} 

char* buscar_instruccion_en(FILE* f, uint32_t pc) {
    char* linea = NULL;
    size_t longitud = 0;
    ssize_t leidos = 0;
    for (int i = 0; i <= pc; i++) {
        leidos = getline(&linea, &longitud, f);
        if (leidos == -1) { // Se alcanzó el final del archivo antes de llegar a la línea 'pc'
            return NULL;
        }
        if(linea[0] == '\n') {
            leidos = getline(&linea, &longitud, f);
        }
    }
    // Cerrar el archivo y devolver la línea leída
    //fclose(f);
    return linea;
}

void archivo_proceso_destroy(archivo_proceso_t* archivo_proceso) {
    free(archivo_proceso->path);
    free(archivo_proceso);
}

void eliminar_archivo_proceso(t_list* archivos_procesos, uint32_t pid_a_finalizar){
    archivo_proceso_t* archivo_proceso = archivo_proceso_por_pid(pid_a_finalizar, archivos_procesos);
    list_remove_element(archivos_procesos, archivo_proceso);
    archivo_proceso_destroy(archivo_proceso);
}
#include "../include/seniales.h"

t_log* logger_sen;

void controlar_seniales(t_log* logger) {
    logger_sen = logger;
    //log_info(logger_sen, "Llegue aca");
    signal(SIGINT, cerrar_seniales);
}

void cerrar_seniales() {
    //log_info(logger_sen, "Llegue aca");
    avisar_desconexion_kernel();
    exit(0);
}

void avisar_desconexion_kernel() {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + string_length(nombre) + 1);
    int offset = 0;
    agregar_opcode(stream, &offset, INTERFAZ_BYE);
    agregar_string(stream, &offset, nombre);
    send(kernel_fd, stream, offset, 0);
    free(stream); 
}

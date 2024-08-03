#include "../include/seniales.h"

t_log* logger_sen;

void controlar_seniales(t_log* logger) {
    logger_sen = logger;
    signal(SIGINT, cerrar_seniales);
}

void cerrar_seniales() {
    exit(0);
}
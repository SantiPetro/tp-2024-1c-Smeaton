#include <stdlib.h>
#include <stdio.h>
#include <../include/main.h>

void get_config(t_config *config)
{
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    cantidad_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    algoritmo_tlb = config_get_string_value(config, "ALGORITMO_TLB");
}

int servers_escuchar()
{
    return 
    server_escuchar(cpu_interrupt_fd, logger_cpu, (procesar_conexion_func_t)procesar_conexion_interrupt, "CPU interrupt") && 
    server_escuchar(cpu_dispatch_fd, logger_cpu, (procesar_conexion_func_t)procesar_conexion_dispatch, "CPU dispatch");
}

void inicializar_semaforos() {
    sem_init(&fin_pedido_recursos, 0, 0);
    flag_sigue_en_exec = 1;
}

void liberar_semaforos() {
    sem_destroy(&fin_pedido_recursos);
}

int main(int argc, char *argv[])
{
    logger_cpu = iniciar_logger("cpu.log", "CPU: ");
    t_config *config_cpu = iniciar_config(argv[1]);
    get_config(config_cpu);

    registros_cpu = inicializar_registros();
    inicializar_semaforos();
    inicializar_tlb();

    controlar_seniales(logger_cpu);
    // Se conecta como cliente a la memoria
    memoria_fd = generar_conexion(logger_cpu, "memoria", ip_memoria, puerto_memoria, config_cpu);
    tamanio_pagina = (int)pedir_tamanio_pagina(memoria_fd);

    // Empieza el servidor dispatch
    cpu_dispatch_fd = iniciar_servidor(logger_cpu, puerto_escucha_dispatch, "CPU dispatch");

    // Empieza el servidor interrupt
    cpu_interrupt_fd = iniciar_servidor(logger_cpu, puerto_escucha_interrupt, "CPU interrupt");
    while (servers_escuchar());

    // TODO: ver como sincronizar el comienzo de cada server y del cliente.

    liberar_semaforos();
    liberar_conexion(memoria_fd);
    terminar_programa(logger_cpu, config_cpu);
    free(registros_cpu);
    return 0;
}

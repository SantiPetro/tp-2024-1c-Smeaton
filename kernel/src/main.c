#include <stdlib.h>
#include <stdio.h>
#include <../include/init.h>
#include <../include/main.h>
#include <../include/consola.h>

void inicializar_valores()
{
    inicializar_listas();
    inicializar_semaforos();
    diccionario_interfaces = dictionary_create();
    pid_siguiente = 1;
    fin_a_proceso_sleep = 0;
    disminuciones_multiprogramacion = 0;
    planificacion_activa = 1;
    reanudar_planificacion = 0;
    ejecuciones = 0;
}

void liberar_valores()
{
    liberar_listas();
    liberar_semaforos();
    liberar_info_interfaces();
}

void liberar_info_interfaces()
{
    interfaz_t *interfaz;
    while ((interfaz = list_get(interfaces, 0)) != NULL)
    {
        list_remove(interfaces, (void *)interfaz);
        list_destroy(interfaz->cola);
        pthread_mutex_destroy(&interfaz->mutex_cola);
        pthread_mutex_destroy(&interfaz->mutex_exec);
        pthread_mutex_destroy(&interfaz->mutex_fin_de_proceso);
        sem_destroy(&interfaz->sem_eliminar_proceso);
        sem_destroy(&interfaz->sem_vuelta);
        free(interfaz->nombre);
        free(interfaz->tipo);
        free(interfaz);
    }
    list_destroy(interfaces);
}

int main(int argc, char *argv[])
{
    logger_kernel = iniciar_logger("kernel.log", "KERNEL: ");
    t_config *config_kernel = iniciar_config(argv[1]);
    //t_config *config_kernel = iniciar_config("kernelse.config");
    get_config(config_kernel);
    inicializar_valores();

    // Se conecta como cliente a la memoria (interrupt)
    memoria_interrupt_fd = generar_conexion(logger_kernel, "memoria", ip_memoria, puerto_memoria, config_kernel);

    // Se conecta como cliente a la memoria (dispatch)
    int memoria_dispatch_fd = generar_conexion(logger_kernel, "memoria", ip_memoria, puerto_memoria, config_kernel);

    empezar_hilo_consola(&hilo_consola, logger_kernel, memoria_dispatch_fd);

    // Se conecta como cliente al CPU dispatch
    cpu_dispatch_fd = generar_conexion(logger_kernel, "CPU dispatch", ip_cpu, puerto_cpu_dispatch, config_kernel);

    // Se conecta como cliente al CPU interrupt
    cpu_interrupt_fd = generar_conexion(logger_kernel, "CPU interrupt", ip_cpu, puerto_cpu_interrupt, config_kernel);

    // Empieza el servidor
    int kernel_fd = iniciar_servidor(logger_kernel, puerto_escucha, "kernel");
    while (server_escuchar(kernel_fd, logger_kernel, (procesar_conexion_func_t)procesar_conexion, "kernel"))
        ;

    pthread_join(hilo_consola, NULL);
    liberar_valores();
    terminar_programa(logger_kernel, config_kernel);
    liberar_conexion(memoria_dispatch_fd);
    liberar_conexion(memoria_interrupt_fd);
    liberar_conexion(cpu_dispatch_fd);
    liberar_conexion(cpu_interrupt_fd);
    return 0;
}
#include <../include/registros.h>

void inicializar_listas()
{
    pcbs_new = list_create();
    pcbs_ready = list_create();
    pcbs_exec = list_create();
    pcbs_exit = queue_create();
    pcbs_ready_prioritarios = list_create();
    interfaces = list_create();
    pcbs_generica = list_create();
    pcbs_stdin = list_create();
    pcbs_stdout = list_create();
    pcbs_dialfs = list_create();
    pcbs_recursos = malloc(cantidad_recursos * sizeof(t_list));
    for (int i = 0; i < cantidad_recursos; i++)
    {
        pcbs_recursos[i] = list_create();
    }
    pids_eliminados = list_create();
}

void liberar_listas()
{
    list_destroy(pcbs_new);
    list_destroy(pcbs_ready);
    list_destroy(pcbs_ready_prioritarios);
    list_destroy(pcbs_exec);
    queue_destroy(pcbs_exit);
    list_destroy(interfaces);
    list_destroy(pcbs_generica);
    list_destroy(pcbs_stdin);
    list_destroy(pcbs_stdout);
    list_destroy(pcbs_dialfs);
    for (int i = 0; i < cantidad_recursos; i++)
    {
        list_destroy(pcbs_recursos[i]);
    }
    free(pcbs_recursos);
    list_destroy(pids_eliminados);
}
void get_config(t_config *config)
{
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    ip_cpu = config_get_string_value(config, "IP_CPU");
    puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    quantum = config_get_int_value(config, "QUANTUM");
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    recursos = config_get_array_value(config, "RECURSOS");
    char **instancias_string = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    cantidad_recursos = cantidadDeRecursos(instancias_string);
    instancias_recursos = malloc(cantidad_recursos * sizeof(int));
    for (int i = 0; i < cantidad_recursos; i++)
    {
        instancias_recursos[i] = atoi(instancias_string[i]);
    }
    string_array_destroy(instancias_string);
}

int cantidadDeRecursos(char **instancias_string)
{
    int cant = 0;
    while (instancias_string[cant] != NULL)
    {
        cant++;
    }
    return cant;
}

void inicializar_semaforos()
{
    pthread_mutex_init(&mutex_new_list, NULL);
    pthread_mutex_init(&mutex_ready_list, NULL);
    pthread_mutex_init(&mutex_ready_prioritario_list, NULL);
    pthread_mutex_init(&mutex_exec_list, NULL);
    pthread_mutex_init(&mutex_exit_queue, NULL);
    pthread_mutex_init(&mutex_generica_list, NULL);
    pthread_mutex_init(&mutex_stdin_list, NULL);
    pthread_mutex_init(&mutex_stdout_list, NULL);
    pthread_mutex_init(&mutex_reanudar_planificacion, NULL);
    pthread_mutex_init(&mutex_generica_exec, NULL);
    pthread_mutex_init(&mutex_stdin_exec, NULL);
    pthread_mutex_init(&mutex_stdout_exec, NULL);
    pthread_mutex_init(&mutex_planificacion_activa, NULL);
    pthread_mutex_init(&mutex_lista_interfaces, NULL);
    pthread_mutex_init(&mutex_disminuciones, NULL);
    mutex_recursos_list = malloc(cantidad_recursos * sizeof(pthread_mutex_t));
    mutex_recursos_instancias = malloc(cantidad_recursos * sizeof(pthread_mutex_t));
    for (int i = 0; i < cantidad_recursos; i++)
    {
        pthread_mutex_init(&mutex_recursos_list[i], NULL);
        pthread_mutex_init(&mutex_recursos_instancias[i], NULL);
    }
    sem_init(&multiprogramacion, 0, grado_multiprogramacion);
    sem_init(&pcb_esperando_ready, 0, 0);
    sem_init(&pcb_esperando_exec, 0, 0);
    sem_init(&pcb_esperando_exit, 0, 0);
    sem_init(&pcb_esperando_generica, 0, 0);
    sem_init(&pcb_esperando_stdin, 0, 0);
    sem_init(&pcb_esperando_stdout, 0, 0);
    sem_init(&pcb_esperando_dialfs, 0, 0);
    sem_init(&vuelta_io_gen_sleep, 0, 0);
    sem_init(&vuelta_io_stdin_read, 0, 0);
    sem_init(&vuelta_io_stdout_write, 0, 0);
    sem_init(&sem_detener_planificacion, 0, 0);
    pcb_esperando_recurso = malloc(cantidad_recursos * sizeof(sem_t));
    for (int i = 0; i < cantidad_recursos; i++)
    {
        sem_init(&pcb_esperando_recurso[i], 0, instancias_recursos[i]);
    }
}

void liberar_semaforos()
{
    pthread_mutex_destroy(&mutex_new_list);
    pthread_mutex_destroy(&mutex_ready_list);
    pthread_mutex_destroy(&mutex_ready_prioritario_list);
    pthread_mutex_destroy(&mutex_exec_list);
    pthread_mutex_destroy(&mutex_exit_queue);
    pthread_mutex_destroy(&mutex_generica_list);
    pthread_mutex_destroy(&mutex_stdin_list);
    pthread_mutex_destroy(&mutex_stdout_list);
    pthread_mutex_destroy(&mutex_reanudar_planificacion);
    pthread_mutex_destroy(&mutex_generica_exec);
    pthread_mutex_destroy(&mutex_stdin_exec);
    pthread_mutex_destroy(&mutex_stdout_exec);
    pthread_mutex_destroy(&mutex_planificacion_activa);
    pthread_mutex_destroy(&mutex_lista_interfaces);
    pthread_mutex_destroy(&mutex_disminuciones);
    for (int i = 0; i < cantidad_recursos; i++)
    {
        pthread_mutex_destroy(&mutex_recursos_list[i]);
        pthread_mutex_destroy(&mutex_recursos_instancias[i]);
    }
    free(mutex_recursos_list);
    sem_destroy(&multiprogramacion);
    sem_destroy(&pcb_esperando_ready);
    sem_destroy(&pcb_esperando_exec);
    sem_destroy(&pcb_esperando_exit);
    sem_destroy(&pcb_esperando_generica);
    sem_destroy(&pcb_esperando_stdin);
    sem_destroy(&pcb_esperando_stdout);
    sem_destroy(&pcb_esperando_dialfs);
    sem_destroy(&vuelta_io_gen_sleep);
    sem_destroy(&vuelta_io_stdin_read);
    sem_destroy(&vuelta_io_stdout_write);
    sem_destroy(&sem_detener_planificacion);
    for (int i = 0; i < cantidad_recursos; i++)
    {
        sem_destroy(&pcb_esperando_recurso[i]);
    }
    free(pcb_esperando_recurso);
}
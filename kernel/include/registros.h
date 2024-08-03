#ifndef REGISTROS_H_
#define REGISTROS_H_
#include <commons/collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <../include/procesos.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/temporal.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

typedef struct {
    proceso_t* proceso;
    t_log* logger;
} nuevo_proceso_t;

typedef struct {
    proceso_t* proceso;
    uint32_t uni_de_trabajo;
} proceso_sleep_t;

typedef struct {
    proceso_t* proceso;
    char* interfaz;
    uint32_t uni_de_trabajo;
    uint32_t registro_direccion;
    uint32_t registro_tamanio;
    char* nombre_archivo;
    uint32_t registro_puntero;
    uint32_t cant_paginas;
    char* direcciones_bytes;
} proceso_a_interfaz_t;

typedef struct {
    char* nombre;
    char* tipo;
    t_list* cola;
    pthread_mutex_t mutex_cola;
    pthread_mutex_t mutex_exec;
    sem_t sem_vuelta;
    sem_t sem_eliminar_proceso;
    int fin_de_proceso;
    pthread_mutex_t mutex_fin_de_proceso;
} interfaz_t;

extern t_log* logger_kernel;
extern char* puerto_memoria;
extern char* ip_memoria;
extern char* puerto_cpu_dispatch;
extern char* ip_cpu;
extern char* puerto_cpu_interrupt;
extern char* puerto_escucha;
extern pthread_t hilo_consola;
extern int quantum;
extern char** recursos;
extern int* instancias_recursos;
extern int cantidad_recursos;
extern char* algoritmo_planificacion;
extern int grado_multiprogramacion;
extern t_list* pcbs_new;
extern t_list* pcbs_ready;
extern t_list* pcbs_ready_prioritarios;
extern t_list* pcbs_exec;
extern t_queue* pcbs_exit;
extern t_list* interfaces;
extern t_list* pcbs_generica;
extern t_list* pcbs_stdin;
extern t_list* pcbs_stdout;
extern t_list* pcbs_dialfs;
extern t_list** pcbs_recursos;
extern pthread_mutex_t mutex_new_list;
extern pthread_mutex_t mutex_ready_list;
extern pthread_mutex_t mutex_ready_prioritario_list;
extern pthread_mutex_t mutex_exec_list;
extern pthread_mutex_t mutex_exit_queue;
extern pthread_mutex_t mutex_generica_list;
extern pthread_mutex_t mutex_generica_exec;
extern pthread_mutex_t mutex_stdin_list;
extern pthread_mutex_t mutex_stdin_exec;
extern pthread_mutex_t mutex_stdout_list;
extern pthread_mutex_t mutex_stdout_exec;
extern pthread_mutex_t mutex_reanudar_planificacion;
extern pthread_mutex_t mutex_planificacion_activa;
extern pthread_mutex_t mutex_lista_interfaces;
extern pthread_mutex_t mutex_disminuciones;
extern pthread_mutex_t* mutex_recursos_list;
extern pthread_mutex_t* mutex_recursos_instancias;
extern sem_t multiprogramacion;
extern sem_t pcb_esperando_ready;
extern sem_t pcb_esperando_exec;
extern sem_t pcb_esperando_exit;
extern sem_t pcb_esperando_generica;
extern sem_t pcb_esperando_stdin;
extern sem_t pcb_esperando_stdout;
extern sem_t pcb_esperando_dialfs;
extern sem_t* pcb_esperando_recurso;
extern sem_t vuelta_io_gen_sleep;
extern sem_t vuelta_io_stdin_read;
extern sem_t vuelta_io_stdout_write;
extern sem_t sem_detener_planificacion;
extern int procesos_activos;
extern uint32_t pid_siguiente;
extern int cpu_dispatch_fd;
extern int cpu_interrupt_fd;
extern int memoria_interrupt_fd;
extern t_dictionary* diccionario_interfaces;
extern void enviar_proceso_io_gen_sleep(proceso_t* proceso,char* interfaz_sleep, uint32_t uni_de_trabajo);
void inicializar_listas();
void liberar_listas();
void inicializar_semaforos();
void liberar_semaforos();
void get_config(t_config* config); 
int cantidadDeRecursos(char** instancias_string);
extern void finalizar_proceso(proceso_t* proceso);
extern void recibir_fin_de_sleep(interfaz_t* interfaz);
extern void mostrar_pids_cola(t_list* ready_list, char* cola);
extern uint32_t _get_pid(proceso_t* proceso);
extern void finalizar_proceso_de_pid(uint32_t pid_proceso);
extern uint32_t pid_a_finalizar;
extern int fin_a_proceso_sleep;
extern void cambiar_grado_de_multiprogramacion(int nuevo_grado_multiprogramacion);
extern int disminuciones_multiprogramacion;
extern void enviar_proceso_a_wait(proceso_t* proceso, char* recurso_wait, uint32_t tiempo_en_cpu, t_temporal* timer);
extern void entrar_a_exit(proceso_t* proceso);
extern void enviar_proceso_a_signal(proceso_t* proceso, char* recurso_signal, uint32_t tiempo_en_cpu, t_temporal* timer);
extern void enviar_proceso_a_interfaz(proceso_a_interfaz_t* proceso_a_interfaz, char* interfaz, void (*hacer_peticion)(proceso_a_interfaz_t*, interfaz_t*));
extern void hacer_io_gen_sleep(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
extern void hacer_io_stdin_read(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
extern void hacer_io_stdout_write(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
extern void hacer_io_fs_create(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
extern void hacer_io_fs_delete(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
extern void hacer_io_fs_truncate(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
extern void hacer_io_fs_write(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
extern void hacer_io_fs_read(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
extern void ejecutar_proceso(proceso_t* proceso, t_log* logger, int quantum);
extern void liberar_cpu();
extern void ingresar_a_exec();
extern char* interfaz_a_consultar;
extern interfaz_t* buscar_interfaz(char* nombre);
extern int planificacion_activa;
extern int reanudar_planificacion;
extern void verificar_detencion_de_planificacion();
extern void iniciar_planificacion();
extern proceso_t* proceso_a_verificar;
extern int ejecuciones;
extern void esperar_llegada_de_proceso_fifo(proceso_t *proceso, t_log *logger, t_temporal* timer);
extern void esperar_llegada_de_proceso_rr_vrr(proceso_t *proceso, t_temporal *timer, t_log *logger);
extern t_list* pids_eliminados;
extern bool pids_iguales(uint32_t pid1, uint32_t pid2);
extern void finalizar_procesos_de_interfaz(char* nombre);
#endif
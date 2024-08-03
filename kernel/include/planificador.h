#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_
#include <../include/procesos.h>
#include <../include/registros.h>
#include <commons/log.h>
#include <../include/protocolo.h>
#include <../include/serializacion.h>
#include <../include/conversores.h>
#include <commons/temporal.h>
#include <commons/string.h>

typedef struct {
    proceso_t* proceso;
    t_temporal* timer;
    t_log* logger;
    int ejecucion;
} interrupcion_proceso_t;


void planificar_nuevo_proceso(void* void_args);
void ejecutar_proceso(proceso_t* proceso, t_log* logger, int quantum);
void enviar_proceso_a_cpu(proceso_t* proceso, t_log* logger);
void esperar_contexto_de_ejecucion(proceso_t *proceso, t_log *logger, t_temporal* timer, uint32_t tiempo_en_cpu);
void agregar_pcb(void* stream, int* offset, proceso_t* proceso);
void esperar_llegada_de_proceso_fifo(proceso_t *proceso, t_log *logger, t_temporal* timer);
void esperar_llegada_de_proceso_rr_vrr(proceso_t *proceso, t_temporal *timer, t_log *logger);
void finalizar_proceso(proceso_t* proceso);
void manejar_interrupcion_de_timer(void *args_void);
void ingresar_a_new(proceso_t* proceso);
void ingresar_a_ready();
void ingresar_a_exec();
void ingresar_a_exit(proceso_t* proceso); 
void liberar_cpu();
void mostrar_pids_cola(t_list* ready_list, char* cola);
void finalizar_proceso_de_pid(uint32_t pid_proceso);
bool tiene_el_pid(proceso_t* proceso);
void buscar_en_cola_y_finalizar_proceso(t_list* cola, pthread_mutex_t mutex); 
void mandar_fin_de_quantum_de(proceso_t *proceso);
void buscar_en_cola_de_bloqueados_y_finalizar_proceso(interfaz_t* interfaz);
void buscar_en_exec_y_finalizar_proceso();
void cambiar_grado_de_multiprogramacion(int nuevo_grado_multiprogramacion);
void entrar_a_exit(proceso_t* proceso);
void desalojar_recursos(proceso_t* proceso);
void buscar_en_colas_de_bloqueados_wait_y_finalizar_proceso();
void verificar_detencion_de_planificacion();
void iniciar_planificacion();
bool es_el_proceso(proceso_t* proceso, proceso_t* proceso_a_verificar);
void verificar_multiprogramacion();
uint32_t _get_pid(proceso_t* proceso);
proceso_t *obtenerSiguienteAReady();
proceso_t *obtenerSiguienteAExec();
void finalizar_procesos_de_interfaz(char* nombre);
#endif
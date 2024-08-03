#ifndef BLOQUEADOS_H_
#define BLOQUEADOS_H_

#include <../include/registros.h>
#include <../include/protocolo.h>
#include <../include/serializacion.h>

void enviar_proceso_io_gen_sleep(proceso_t* proceso,char* interfaz_sleep, uint32_t uni_de_trabajo);
int estaConectada(char* interfaz);
void volver_a_ready(proceso_t* proceso);
void enviar_proceso_a_wait(proceso_t* proceso, char* recurso_wait, uint32_t tiempo_en_cpu, t_temporal* timer);
int posicion_de_recurso(char* recurso);
t_list* lista_de_recurso(char* recurso);
bool hay_recursos_de(char* recurso);
void pedir_recurso(char *recurso_wait, proceso_t* proceso, uint32_t pid_proceso);
bool existe_recurso(char* recurso);
void enviar_proceso_a_signal(proceso_t* proceso, char* recurso_signal, uint32_t tiempo_en_cpu, t_temporal* timer);
void devolver_recurso(char* recurso_signal, proceso_t* proceso);
void enviar_proceso_a_interfaz(proceso_a_interfaz_t* proceso_a_interfaz, char* tipo_interfaz, void (*hacer_peticion)(proceso_a_interfaz_t*, interfaz_t*));
void hacer_io_gen_sleep(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
void hacer_io_stdin_read(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
void hacer_io_stdout_write(proceso_a_interfaz_t* proceso_interfaz, interfaz_t* interfaz);
void hacer_io_fs_create(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
void hacer_io_fs_delete(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
void hacer_io_fs_truncate(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
void hacer_io_fs_write(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
void hacer_io_fs_read(proceso_a_interfaz_t *proceso_interfaz, interfaz_t *interfaz);
void recibir_fin_de_peticion(interfaz_t* interfaz);
void volver_a_exec(proceso_t* proceso, uint32_t tiempo_en_cpu, t_temporal* timer);
interfaz_t* buscar_interfaz(char* nombre);
bool es_la_interfaz(interfaz_t* interfaz);
bool pids_iguales(uint32_t pid1, uint32_t pid2);
#endif
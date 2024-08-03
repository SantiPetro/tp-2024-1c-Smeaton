#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <../include/init.h>
#include <../include/conexion.h>
#include <../include/serializacion.h>
#include <semaphore.h>
#include <../include/tlb.h>
#include <../include/seniales.h>

int cpu_dispatch_fd;
int cpu_interrupt_fd;
t_log* logger_cpu;
int memoria_fd;
char* puerto_memoria;
char* ip_memoria;
char* puerto_escucha_dispatch;
char* puerto_escucha_interrupt;
int cantidad_entradas_tlb;
char* algoritmo_tlb;
int tamanio_pagina;
int servers_escuchar();
registros_t* registros_cpu;
sem_t fin_pedido_recursos;
void inicializar_semaforos();
void liberar_semaforos();
int flag_sigue_en_exec;
tlb_t* tlb;
t_queue* cola_fifo_tlb;


#endif
#ifndef PROCESOS_H_
#define PROCESOS_H_

#include <stdint.h>

typedef struct
{
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
} registros_t;

typedef struct
{
    uint32_t pid;
    uint32_t quantum;
    uint8_t* recursos;
    registros_t* registros;
} proceso_t;

typedef struct
{
    uint32_t pid;
    uint32_t quantum;
    registros_t* registros;
} pcb_t;

registros_t* inicializar_registros();

#endif
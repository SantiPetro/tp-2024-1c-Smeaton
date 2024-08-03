#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_
#include <../include/protocolo.h>
#include <commons/log.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint32_t size;
    void* stream;
} t_buffer;

typedef struct
{
    op_code op_code;
    t_buffer* buffer;
} t_paquete;

void agregar_opcode(void* stream, int* offset, op_code op_code);
void agregar_uint32_t(void* stream, int* offset, uint32_t uint32);
void agregar_uint16_t(void* stream, int* offset, uint16_t uint16);
void agregar_uint8_t(void* stream, int* offset, uint8_t uint8);
void agregar_string(void* stream, int* offset, char* string);
void agregar_string_sin_barra0(void* stream, int* offset, char* string);
void agregar_segun_cant_bytes(void* stream, int* offset, uint32_t valor, uint32_t cant_bytes);

#endif
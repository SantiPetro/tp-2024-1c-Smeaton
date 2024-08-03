#include <../include/serializacion.h>

void agregar_opcode(void* stream, int* offset, op_code op_code) {
    memcpy(stream, &op_code, sizeof(op_code));
    *offset += sizeof(op_code);
}

void agregar_uint32_t(void* stream, int* offset, uint32_t uint32) {
    memcpy(stream + (*offset), &uint32, sizeof(uint32_t));
    *offset += sizeof(uint32_t);
}  

void agregar_uint16_t(void* stream, int* offset, uint16_t uint16) {
    memcpy(stream + (*offset), &uint16, sizeof(uint16_t));
    *offset += sizeof(uint16_t);
} 

void agregar_uint8_t(void* stream, int* offset, uint8_t uint8) {
    memcpy(stream + (*offset), &uint8, sizeof(uint8_t));
    *offset += sizeof(uint8_t);
} 

void agregar_segun_cant_bytes(void* stream, int* offset, uint32_t valor, uint32_t cant_bytes) {
    memcpy(stream + (*offset), &valor, cant_bytes);
    *offset += cant_bytes;
}

void agregar_string(void* stream, int* offset, char* string) {
    size_t tamanio = string_length(string) + 1;
    memcpy(stream + (*offset), &tamanio, sizeof(uint32_t));
    *offset += sizeof(uint32_t);
    memcpy((char*) stream + (*offset), string, tamanio);
    *offset += tamanio;
}

void agregar_string_sin_barra0(void* stream, int* offset, char* string) {
    uint16_t tamanio = string_length(string);
    memcpy(stream + (*offset), &tamanio, sizeof(uint16_t));
    *offset += sizeof(uint16_t);
    memcpy((char*) stream + (*offset), string, tamanio);
    *offset += tamanio;
}
#include <../include/procesos.h>

registros_t* inicializar_registros() {
    registros_t* registros = malloc(sizeof(registros_t));
    registros->AX  = 0;
    registros->BX  = 0;
    registros->CX  = 0;
    registros->DX  = 0;
    registros->EAX = 0;
    registros->EBX = 0;
    registros->ECX = 0;
    registros->EDX = 0;
    registros->DI  = 0;
    registros->PC  = 0;
    registros->SI  = 0;
    return registros;
}
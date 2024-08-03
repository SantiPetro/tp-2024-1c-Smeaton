#ifndef MMU_H_
#define MMU_H_
#include <stdint.h>

extern int tamanio_pagina;
uint32_t pagina_direccion_logica(uint32_t direccion_logica);
uint16_t desplazamiento_direccion_logica(uint32_t direccion_logica);
uint8_t cantidad_paginas_enviar(int cantidad_bytes, uint32_t direccion_logica);
#endif

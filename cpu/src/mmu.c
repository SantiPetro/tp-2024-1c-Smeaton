#include <../include/mmu.h>

uint32_t pagina_direccion_logica(uint32_t direccion_logica) {
    return (uint32_t)floor(direccion_logica/tamanio_pagina);
}

uint16_t desplazamiento_direccion_logica(uint32_t direccion_logica) {
    return (uint16_t)(direccion_logica - pagina_direccion_logica(direccion_logica)*tamanio_pagina);
}

uint8_t cantidad_paginas_enviar(int cantidad_bytes, uint32_t direccion_logica) {
    int bytes_restantes_en_pagina = tamanio_pagina - desplazamiento_direccion_logica(direccion_logica);
    if (cantidad_bytes <= bytes_restantes_en_pagina) {
        return 1;
    }
    int bytes_restantes = cantidad_bytes - bytes_restantes_en_pagina;
    int paginas_adicionales = (bytes_restantes + tamanio_pagina - 1) / tamanio_pagina;
    return 1 + paginas_adicionales;
}


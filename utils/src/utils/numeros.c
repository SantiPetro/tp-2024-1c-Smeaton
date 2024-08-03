#include <../include/numeros.h>


int cant_digitos(uint32_t numero) {
    if (numero == 0) return 1;
    int digitos = 0;
    while (numero > 0) {
        numero /= 10;
        digitos++;
    }
    return digitos;
}

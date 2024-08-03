#include <../include/proceso.h>

proceso_t *crear_pcb(uint32_t pid)
{
    proceso_t *nuevo_pcb = malloc(sizeof(proceso_t));
    nuevo_pcb->pid = pid;
    nuevo_pcb->quantum = (uint32_t)quantum;
    nuevo_pcb->registros = inicializar_registros();
    nuevo_pcb->recursos = malloc(cantidad_recursos * sizeof(uint8_t));
    for (int i = 0; i < cantidad_recursos; i++)
    {
        nuevo_pcb->recursos[i] = 0;
    }
    return nuevo_pcb;
}
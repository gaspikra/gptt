#include "pcb.h"
#include <string.h>
#include <stdlib.h> 
#include <sys/socket.h> 



t_pcb* crear_pcb(uint32_t contador_procesos) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = contador_procesos;
    pcb->pc = 0;
     pcb->path = NULL;
    pcb->quantum = 0;
    pcb->estado = NEW;
    pcb->registros.ax = 0;
    pcb->registros.bx = 0;
    pcb->registros.cx = 0;
    pcb->registros.dx = 0;
    pcb->registros.dx = 0;
    pcb->registros.eax = 0;
    pcb->registros.ebx = 0;
    pcb->registros.ecx = 0;
    return pcb;
}


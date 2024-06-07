#ifndef PCB_H_
#define PCB_H_


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


typedef struct registros_cpu{
	uint8_t ax, bx, cx, dx;
    uint32_t eax,ebx,ecx,edx,si,di;
}t_registros;


typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,
	EXITT,
	ERROR
} estado_code;

typedef struct
{
	uint32_t pid;
	uint32_t pc;
    char * path;
    uint32_t quantum;
    uint32_t estado;
    t_registros registros;
} t_pcb;





#endif // PCB_H_


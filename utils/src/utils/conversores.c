#include <../include/conversores.h>

op_code string_to_opcode(char* string) {
    if (strcmp(string, "SET") == 0) return SET;
    else if (strcmp(string, "MOV_IN") == 0) return MOV_IN;
    else if (strcmp(string, "MOV_OUT") == 0) return MOV_OUT;
    else if (strcmp(string, "SUM") == 0) return SUM;
    else if (strcmp(string, "SUB") == 0) return SUB;
    else if (strcmp(string, "JNZ") == 0) return JNZ;
    else if (strcmp(string, "RESIZE") == 0) return RESIZE;
    else if (strcmp(string, "COPY_STRING") == 0) return COPY_STRING;
    else if (strcmp(string, "WAIT") == 0) return WAIT;
    else if (strcmp(string, "SIGNAL") == 0) return SIGNAL;
    else if (strcmp(string, "IO_GEN_SLEEP") == 0) return IO_GEN_SLEEP;
    else if (strcmp(string, "IO_STDIN_READ") == 0) return IO_STDIN_READ;
    else if (strcmp(string, "IO_STDOUT_WRITE") == 0) return IO_STDOUT_WRITE;
    else if (strcmp(string, "IO_FS_CREATE") == 0) return IO_FS_CREATE;
    else if (strcmp(string, "IO_FS_DELETE") == 0) return IO_FS_DELETE;
    else if (strcmp(string, "IO_FS_TRUNCATE") == 0) return IO_FS_TRUNCATE;
    else if (strcmp(string, "IO_FS_WRITE") == 0) return IO_FS_WRITE;
    else if (strcmp(string, "IO_FS_READ") == 0) return IO_FS_READ;
    else if (strcmp(string, "EXIT") == 0) return EXIT;
    else if (strcmp(string, "INICIAR_PROCESO") == 0) return INICIAR_PROCESO;
    else if (strcmp(string, "FINALIZAR_PROCESO") == 0) return FINALIZAR_PROCESO;
    else if (strcmp(string, "GENERICA") == 0) return GENERICA;
    else if (strcmp(string, "STDIN") == 0) return STDIN;
    else if (strcmp(string, "STDOUT") == 0) return STDOUT;
    else if (strcmp(string, "DIALFS") == 0) return DIALFS;
    else if (strcmp(string, "ENVIAR_PCB") == 0) return ENVIAR_PCB;
    else if (strcmp(string, "INTERRUMPIR") == 0) return INTERRUMPIR;
    else if (strcmp(string, "FETCH") == 0) return FETCH;
    else if (strcmp(string, "DATOS_PCB") == 0) return DATOS_PCB;
    else if (strcmp(string, "MSG") == 0) return MSG;
    else if (strcmp(string, "TIMER") == 0) return TIMER;
    else if (strcmp(string, "RECURSO_INVALIDO") == 0) return RECURSO_INVALIDO;
    else if (strcmp(string, "OUTOFMEMORY") == 0) return OUTOFMEMORY;
}
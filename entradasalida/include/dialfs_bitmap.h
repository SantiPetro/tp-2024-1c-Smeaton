#ifndef DIALFS_BITMAP_H_
#define DIALFS_BITMAP_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>

extern FILE* f_bitmap;
extern int block_count;
extern t_log* logger_io;
extern char* path_base_dialfs;

int buscar_y_setear_bloque_libre();


#endif
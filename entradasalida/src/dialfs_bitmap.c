#include <../include/dialfs_bitmap.h>

int buscar_y_setear_bloque_libre() {
    char* path = string_new();
    string_append(&path, path_base_dialfs);
    string_append(&path, "/bitmap.dat");
    f_bitmap = fopen(path, "r+b");
    size_t tamanio_archivo = (block_count+7)/8;
    char* lectura_f_bloques = malloc(tamanio_archivo);
    fseek(f_bitmap, 0, SEEK_SET);
    fread(lectura_f_bloques, sizeof(char), tamanio_archivo, f_bitmap);
    t_bitarray* bitarray = bitarray_create(lectura_f_bloques,tamanio_archivo);
    for (int i = 0; i < block_count; i++) {
        if (!bitarray_test_bit(bitarray, i)) {
            bitarray_set_bit(bitarray, i);
            fseek(f_bitmap, 0, SEEK_SET);
            fwrite(bitarray->bitarray, sizeof(char), tamanio_archivo, f_bitmap);
            free(path);
            free(lectura_f_bloques);
            fclose(f_bitmap);
            bitarray_destroy(bitarray);
            return i;
        }
    }
    free(lectura_f_bloques);
    free(path);
    bitarray_destroy(bitarray);
    fclose(f_bitmap);
    return -1;
}
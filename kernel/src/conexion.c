#include <../include/conexion.h>

uint32_t tam;
char *nombre;
interfaz_t *interfaz;
void procesar_conexion(void *args_void)
{
    conexion_args_t *args = (conexion_args_t *)args_void;
    int socket_cliente = args->socket_cliente;
    t_log *logger = args->logger;
    free(args);

    op_code opcode;
    while (socket_cliente != 1)
    {
        if ((recv(socket_cliente, &opcode, sizeof(op_code), 0)) != sizeof(op_code))
        {
            //log_info(logger, "Tiro error");
            return;
        }
        log_info(logger_kernel, "OPCODE: %d", opcode);
        uint32_t tam;
        char *nombre;
        switch (opcode)
        {
        case GENERICA:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            guardar_interfaz(nombre, "GENERICA");
            log_info(logger_kernel, "%s", nombre);
            log_info(logger, "Se conectó la interfaz genérica");
            conectar_interfaz(nombre, socket_cliente);
            free(nombre);
            break;
        case STDIN:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            guardar_interfaz(nombre, "STDIN");
            conectar_interfaz(nombre, socket_cliente);
            free(nombre);
            break;
        case STDOUT:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            guardar_interfaz(nombre, "STDOUT");
            conectar_interfaz(nombre, socket_cliente);
            free(nombre);
            break;
        case DIALFS:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            guardar_interfaz(nombre, "DIALFS");
            conectar_interfaz(nombre, socket_cliente);
            free(nombre);
            break;
        case INTERFAZ_BYE:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            desconectar_interfaz(nombre);
            finalizar_procesos_de_interfaz(nombre);
            free(nombre);
            break;
        case FIN_DE_SLEEP:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            interfaz = buscar_interfaz(nombre);
            sem_post(&interfaz->sem_vuelta);
            free(nombre);
            break;
        case FIN_DE_STDIN:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            interfaz = buscar_interfaz(nombre);
            sem_post(&interfaz->sem_vuelta);
            free(nombre);
            break;
        case FIN_DE_STDOUT:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            interfaz = buscar_interfaz(nombre);
            sem_post(&interfaz->sem_vuelta);
            free(nombre);
            break;
        case FIN_DE_DIALFS:
            recv(socket_cliente, &tam, sizeof(uint32_t), 0);
            nombre = malloc(tam);
            recv(socket_cliente, nombre, tam, 0);
            interfaz = buscar_interfaz(nombre);
            sem_post(&interfaz->sem_vuelta);
            free(nombre);
            break;
        default:
        }
    }
    return;
}

void conectar_interfaz(char *interfaz, int socket)
{
    dictionary_put(diccionario_interfaces, interfaz, (void *)socket);
}

void desconectar_interfaz(char *interfaz)
{
    dictionary_remove(diccionario_interfaces, interfaz);
}

void guardar_interfaz(char *nombre, char *tipo)
{
    interfaz_t *nueva_interfaz = malloc(sizeof(interfaz_t));
    nueva_interfaz->nombre = malloc(string_length(nombre) + 1);
    nueva_interfaz->tipo = malloc(string_length(tipo) + 1);
    memcpy(nueva_interfaz->nombre, nombre, string_length(nombre) + 1);
    memcpy(nueva_interfaz->tipo, tipo, string_length(tipo) + 1);
    nueva_interfaz->cola = list_create();
    pthread_mutex_init(&nueva_interfaz->mutex_cola, NULL);
    pthread_mutex_init(&nueva_interfaz->mutex_exec, NULL);
    pthread_mutex_init(&nueva_interfaz->mutex_fin_de_proceso, NULL);
    sem_init(&nueva_interfaz->sem_vuelta, 0, 0);
    sem_init(&nueva_interfaz->sem_eliminar_proceso, 0, 0);
    nueva_interfaz->fin_de_proceso = 0;
    pthread_mutex_lock(&mutex_lista_interfaces);
    list_add(interfaces, nueva_interfaz);
    pthread_mutex_unlock(&mutex_lista_interfaces);
}
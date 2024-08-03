#include <../include/sockets.h>

int iniciar_servidor(t_log* logger, char* puerto, char* nombre_server)
{
   int socket_servidor;
   int yes = 1;
   struct addrinfo hints, *servinfo, *p;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   getaddrinfo(NULL, puerto, &hints, &servinfo);

   socket_servidor = socket(servinfo->ai_family,
                            servinfo->ai_socktype,
                            servinfo->ai_protocol);

    if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

   bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
   listen(socket_servidor, SOMAXCONN);
   freeaddrinfo(servinfo);
   log_info(logger, "Servidor %s a la espera de un cliente!", nombre_server);
   return socket_servidor;
}

int server_escuchar(int socket_server, t_log* logger, procesar_conexion_func_t procesar_conexion_func, char* nombre_server) {
    int socket_cliente = esperar_cliente(socket_server, logger, nombre_server);
    if(socket_cliente != -1) {
        pthread_t hilo;
        conexion_args_t* args = malloc(sizeof(conexion_args_t));
        args->logger = logger;
        args->socket_cliente = socket_cliente;
        pthread_create(&hilo, NULL, procesar_conexion_func, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

int esperar_cliente(int socket_servidor, t_log* logger, char* nombre_server) {
   int socket_cliente = accept(socket_servidor, NULL, NULL);
   log_info(logger, "Se conecto un cliente a %s!", nombre_server);
   return socket_cliente;
}

void liberar_conexion(int socket_cliente) {
   close(socket_cliente);
}

int generar_conexion(t_log* logger, char* nombre_server, char* ip, char* puerto, t_config* config) {
    int fd = 0;
    if(!(fd = crear_conexion(logger, nombre_server, ip, puerto))){
        terminar_programa(logger, config);
        exit(3);
    }
    return fd;
}

int crear_conexion(t_log* logger, const char* nombre_server, char* ip, char* puerto) {
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if(socket_cliente == -1) {
        log_error(logger, "Error creando el socket para %s:%s", ip, puerto);
        return 0;
    }

    if(connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar (a %s)\n", nombre_server);
        freeaddrinfo(servinfo);
        return 0;
    } else
        log_info(logger, "Cliente conectado (a %s) en %s:%s \n", nombre_server, puerto, ip);

    freeaddrinfo(servinfo);

    return socket_cliente;
}


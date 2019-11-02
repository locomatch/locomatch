#include "libmuse.h"
#include "sockets.h"


int id_global; //Seria algo global?
char sep[2] = {' '};


//NO le di uso al ID. Deberia usarlo dentro del paquete para que MUSE detecte quien es?
int muse_init(int id, char* ip, int puerto){
    id_global = id;
    socket_cliente = crear_conexion_con_servidor(ip, char* puerto);
// * @return Si pasa un error, retorna -1. Si se inicializó correctamente, retorna 0.
}


void muse_close(){
    close(socket_cliente);
}


uint32_t muse_alloc(uint32_t tam){

    enviar_mensaje(MUSE_ALLOC, tam, int socket_cliente);

// * @return La dirección de la memoria reservada.
}


void muse_free(uint32_t dir){
//Tengo que cambiar el formato de dir?
    enviar_mensaje(MUSE_FREE, dir, int socket_cliente);
}


int muse_get(void* dst, uint32_t src, size_t n){
    char* buffer;
    int tot_len = strlen(dst) + strlen(src) + strlen(n) + 2;

    buffer = malloc(tot_len);
    strcpy(buffer, dst);
    strcat(buffer, sep);
    strcat(buffer, src);
    strcat(buffer, sep);
    strcat(buffer, n);

    enviar_mensaje(MUSE_GET, buffer, int socket_cliente);
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}


int muse_cpy(uint32_t dst, void* src, int n){
    char* buffer;
    int tot_len = strlen(dst) + strlen(src) + strlen(n) + 2;

    buffer = malloc(tot_len);
    strcpy(buffer, dst);
    strcat(buffer, sep);
    strcat(buffer, src);
    strcat(buffer, sep);
    strcat(buffer, n);

    enviar_mensaje(MUSE_CPY, buffer, int socket_cliente);
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}



uint32_t muse_map(char *path, size_t length, int flags){
    char* buffer;
    int tot_len = strlen(dst) + strlen(src) + strlen(n) + 2;

    buffer = malloc(tot_len);
    strcpy(buffer, path);
    strcat(buffer, sep);
    strcat(buffer, length);
    strcat(buffer, sep);
    strcat(buffer, flasg);

    enviar_mensaje(MUSE_MAP, buffer, int socket_cliente);
//@return Retorna la posición de memoria de MUSE mappeada.
}


int muse_sync(uint32_t addr, size_t len){
    char* buffer;
    int tot_len = strlen(dst) + strlen(src) + strlen(n) + 2;

    buffer = malloc(tot_len);
    strcpy(buffer, addr);
    strcat(buffer, sep);
    strcat(buffer, len);


    enviar_mensaje(MUSE_SYNC, buffer, int socket_cliente);
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}

int muse_unmap(uint32_t dir){
//Tengo que cambiar el formato de dir?
    enviar_mensaje(MUSE_UNMAP, dir, int socket_cliente);
// * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}
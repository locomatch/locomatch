#include "libmuse.h"
#include "sockets.h"


//int id_global; //Seria algo global?
//char sep[2] = {' '};

int socket_cliente = 0;


//NO le di uso al ID. Deberia usarlo dentro del paquete para que MUSE detecte quien es?
//Me parece que al crear la conexion correctamente se le deberia enviar los datos para identificar al proceso
int muse_init(int id, char* ip, int puerto){
printf("Conectando a Muse... \nIP: %s \nPUERTO: %i", ip, puerto);
    socket_cliente = crear_conexion_con_servidor(ip, puerto);
    if(socket_cliente == 0){
        return -1;
    }
    else{
        return 0;
    }

// * @return Si pasa un error, retorna -1. Si se inicializó correctamente, retorna 0.
}


void muse_close(){
    close(socket_cliente);
}


uint32_t muse_alloc(uint32_t tam){

//    enviar_mensaje(MUSE_ALLOC, tam, socket_cliente);

	t_paquete *create = crear_paquete(MUSE_ALLOC);
	agregar_a_paquete(create, &tam, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);

    recibir_mensaje(socket_cliente);

    char* rta = recibir_mensaje(int socket_cliente);
    printf("Direccion de la memoria reservada:");
    return rta;

// * @return La dirección de la memoria reservada.
}


void muse_free(uint32_t dir){
//Tengo que cambiar el formato de dir?
//    enviar_mensaje(MUSE_FREE, dir, socket_cliente);

	t_paquete *create = crear_paquete(MUSE_FREE);
	agregar_a_paquete(create, &dir, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);
//No pide RESPUESTA. SE la doy igual?
    char* rta = recibir_mensaje(socket_cliente);
    return rta; //0 o -1

}



int muse_get(void* dst, uint32_t src, size_t n){

	t_paquete *create = crear_paquete(MUSE_GET);
	agregar_a_paquete(create, &dst, sizeof(dst)); //ACA HAY ALGO MAL
    agregar_a_paquete(create, &src, sizeof(uint32_t));
    agregar_a_paquete(create, &n, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);

    char* rta = recibir_mensaje(socket_cliente);
    return rta; //0 o -1


//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}


int muse_cpy(uint32_t dst, void* src, int n){
	t_paquete *create = crear_paquete(MUSE_CPY);
	agregar_a_paquete(create, &dst, sizeof(uint32_t));
    agregar_a_paquete(create, &src, sizeof(src)); //ACA HAY ALGO MAL
    agregar_a_paquete(create, &n, sizeof(int));
	enviar_paquete(create, socket_cliente);

    char* rta = recibir_mensaje(socket_cliente);
    return rta; //0 o -1
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}



uint32_t muse_map(char *path, size_t length, int flags){
	t_paquete *create = crear_paquete(MUSE_MAP);
	agregar_a_paquete(create, &path, sizeof(uint32_t));
    agregar_a_paquete(create, &lenght, sizeof(size_t));
    agregar_a_paquete(create, &flags, sizeof(int));
	enviar_paquete(create, socket_cliente);

    char* rta = recibir_mensaje(socket_cliente);
    printf("Posicion de memoria de MUSE mappeada:");
    return rta; //0 o -1
//@return Retorna la posición de memoria de MUSE mappeada.
}


int muse_sync(uint32_t addr, size_t len){
	t_paquete *create = crear_paquete(MUSE_SYNC);
	agregar_a_paquete(create, &addr, sizeof(uint32_t));
    agregar_a_paquete(create, &len, sizeof(size_t));
	enviar_paquete(create, socket_cliente);

    char* rta = recibir_mensaje(socket_cliente);
    return rta; //0 o -1
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}

int muse_unmap(uint32_t dir){
	t_paquete *create = crear_paquete(MUSE_UNMAP);
	agregar_a_paquete(create, &dir, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);

    recibir_mensaje(socket_cliente);

    char* rta = recibir_mensaje(socket_cliente);
    return rta; //0 o -1
// * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}



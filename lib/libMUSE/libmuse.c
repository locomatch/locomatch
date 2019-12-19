#include "libmuse.h"
#include <commons/string.h>
//#include "sockets.h"

#include <linuse/sockets/sockets.h>




int socket_cliente = 0;




int muse_init(int id, char* ip, int puerto){
printf("Conectando a Muse... \nIP: %s \nPUERTO: %i", ip, puerto);
char* puertoChar = string_itoa(puerto);
    socket_cliente = crear_conexion_con_servidor(ip, puertoChar); //ESTA BIEN ESTO?
//NO le di uso al ID. Deberia usarlo dentro del paquete para que MUSE detecte quien es?
//Me parece que al crear la conexion correctamente se le deberia enviar los datos para identificar al proceso
    if(socket_cliente == 0){
        return -1;
    }
    else{
        return 0;
    }

}


void muse_close(){
printf("Desconectandose de Muse...");
//ENVIAR NUEVO CODIGO DE OPERACION A MUSE MOSTRANDO EL MUSE_CLOSE
    close(socket_cliente);
printf("Desconectado");
}


uint32_t muse_alloc(uint32_t tam){

    char* tamChar = string_itoa(tam);

	t_paquete *create = crear_paquete(MUSE_ALLOC);
	agregar_a_paquete(create, tamChar, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);


    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    printf("Direccion de la memoria reservada: %s", rta);
    return atoi(rta);
}


void muse_free(uint32_t dir){

	t_paquete *create = crear_paquete(MUSE_FREE);
    char* dirChar = string_itoa(dir);
	agregar_a_paquete(create, dirChar, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);
}



int muse_get(void* dst, uint32_t src, size_t n){

    char* srcChar = string_itoa(src);
    char* nChar = string_itoa(n);

	t_paquete *create = crear_paquete(MUSE_GET);
	agregar_a_paquete(create, &dst, sizeof(int)); //ACA HAY ALGO MAL
    agregar_a_paquete(create, srcChar, sizeof(uint32_t));
    agregar_a_paquete(create, nChar, sizeof(size_t)); //uint32_t ??
	enviar_paquete(create, socket_cliente);

    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    return atoi(rta);

}


int muse_cpy(uint32_t dst, void* src, int n){

    char* dstChar = string_itoa(dst);
    char* nChar = string_itoa(n);

	t_paquete *create = crear_paquete(MUSE_CPY);
	agregar_a_paquete(create, dstChar, sizeof(uint32_t));
    agregar_a_paquete(create, &src, sizeof(int)); //ACA HAY ALGO MAL
    agregar_a_paquete(create, nChar, sizeof(int));
	enviar_paquete(create, socket_cliente);

    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    return atoi(rta);
}

//Creo que tampoco?

uint32_t muse_map(char *path, size_t length, int flags){

char* lengthChar = string_itoa(length);
char* flagsChar = string_itoa(flags);

	t_paquete *create = crear_paquete(MUSE_MAP);
	agregar_a_paquete(create, &path, sizeof(uint32_t));
    agregar_a_paquete(create, lengthChar, sizeof(size_t));
    agregar_a_paquete(create, flagsChar, sizeof(int));
	enviar_paquete(create, socket_cliente);

    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    printf("Posicion de memoria de MUSE mappeada:");
    return atoi(rta);

}

//X AHORA NO DEBERIAN SER NECESARIOS

int muse_sync(uint32_t addr, size_t len){
	t_paquete *create = crear_paquete(MUSE_SYNC);
	agregar_a_paquete(create, &addr, sizeof(uint32_t));
    agregar_a_paquete(create, &len, sizeof(size_t));
	enviar_paquete(create, socket_cliente);

    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    return atoi(rta);
//@return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}

int muse_unmap(uint32_t dir){
	t_paquete *create = crear_paquete(MUSE_UNMAP);
	agregar_a_paquete(create, &dir, sizeof(uint32_t));
	enviar_paquete(create, socket_cliente);

    recibir_mensaje(socket_cliente);

    int size;
    char* rta = recibir_buffer(&size, socket_cliente);
    return atoi(rta);
// * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
}



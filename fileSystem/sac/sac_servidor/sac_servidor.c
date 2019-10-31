#include "sac_servidor.h"

int main(void) {
   printf("Corriendo el servidor\n");
   run_server();
}

char* action_open(package_open* package) {
    printf("Se recibio una accion open\n");
    return "je";
}
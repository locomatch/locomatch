#include "server.h"
#include "parser.h"

// Funcion que va a usar el server
int run_server(){
    
    struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE

    /* Necesitamos un socket que escuche las conecciones entrantes */
	int listenningSocket;
	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int activado = 1;
    setsockopt(listenningSocket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    if(bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen)){
        printf("el servidor se cayo por un error en bind()");
        exit(1);
    }

	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	//escucha al socket con un queue de 5 (BACKLOG) coneccioens
    listen(listenningSocket, BACKLOG);		// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	struct sockaddr_in cli_addr;
	int clientNumber;
	char buffer[3000];

	while(1){  

        clientNumber = sizeof(cli_addr);
        int newsockfd = accept(listenningSocket, (struct sockaddr *) &cli_addr, &clientNumber);
                
        if(newsockfd<0){
            printf("el servidor se cayo por un error en accept()\n");
        }


        printf("El servidor se conecto exitosamente\n");
        while (1){

            if(read(newsockfd, buffer, 3000)){
                
				printf("se va a parsear\n");
                char* responce = parse_input(buffer);

                write(newsockfd,responce,strlen(responce)+1);

                free(responce);
            
            }else{
                break;
            }//SI el cliente se desconecta deja de leer para ir a por otro cliente

        }    
    
    }
    
	return 0;
}

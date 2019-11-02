#include "server.h"
#include "parser.h"

// Funcion que va a usar el server
void* run_server(void* args){

	server_info* serverInfo = args;
    
	int sockfd, clilentNumber;
	struct sockaddr_in serverAddress, cli_addr;
    char buffer[3000];

	int portNumber = htons(serverInfo->portNumber);

	//esta esturctura esta en in.h y tiene la informacion necesaria para hacer bind() al sv 
	serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = portNumber;
    serverAddress.sin_addr.s_addr = INADDR_ANY;

	//argv0 que la coneccion es por medio de sockets, arv1 tipo TCP , argv2 y devuelve un fd (file decriptor)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int activado = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    if(bind(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress))){

        log_error(serverInfo->logger, "el servidor se cayo por un error en bind()");
        exit(1);
        
    }

	//escucha al socket con un queue de 5 coneccioens
    listen(sockfd, 5);
    log_info(serverInfo->logger, "El servidor se inicializo exitosamente");
    
    while(1){  

        clilentNumber = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilentNumber);
                
        if(newsockfd<0){

            log_error(serverInfo->logger, "el servidor se cayo por un error en accept()");
            
        }

        log_info(serverInfo->logger, "El servidor se conecto exitosamente");
        while (1){
            if(read(newsockfd, buffer, 3000)){
                //log_info(serverInfo->logger, buffer);
                char* response = parse_input(buffer);

                write(newsockfd,response,strlen(response)+1);

                free(response);
            
            }else{
                 break;

             }//SI el cliente se desconecta deja de leer para ir a por otro cliente

         }    
    
    }
	/*
	////////////////////////////////////////////////////////
    struct addrinfo hints;
	struct addrinfo *serverInfo;

	log_info(server_info->logger, "peroo");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	log_info(server_info->logger, "zapzapzap");

	getaddrinfo(NULL,(char*) portNumber, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE

	log_info(server_info->logger, "tapatapatapa");
	printf("vamos por aca");

    //Necesitamos un socket que escuche las conecciones entrantes 
	int listenningSocket;
	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int activado = 1;
    setsockopt(listenningSocket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    if(bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen)){
        log_error(server_info->logger, "el servidor se cayo por un error en bind()");
        exit(1);
    }

	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	//escucha al socket con un queue de 5 (BACKLOG) coneccioens
    listen(listenningSocket, BACKLOG);		// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	struct sockaddr_in cli_addr;
	socklen_t clientNumber;
	char buffer[3000];

	while(1){  

        clientNumber = sizeof(cli_addr);
        int newsockfd = accept(listenningSocket, (struct sockaddr *) &cli_addr, &clientNumber);
                
        if(newsockfd<0){
			log_error(server_info->logger, "el servidor se cayo por un error en accept()");
        }

		log_info(server_info->logger, "El servidor se conecto exitosamente");
        //printf("El servidor se conecto exitosamente\n");
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
	////////////////////////////////
	*/
    
	return 0;
}

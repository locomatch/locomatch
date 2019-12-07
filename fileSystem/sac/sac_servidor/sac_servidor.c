#include "sac_servidor.h"

t_log* logger;

int main(void) {
    printf("Corriendo el servidor\n");

    /*CONFIG*/
    t_config* config = config_create("config");
    int PORT = config_get_int_value(config, "LISTEN_PORT");

    /*LOG*/
    char* LOGPATH = strdup(config_get_string_value(config, "LOG_PATH"));
    logger = log_create(LOGPATH, "Sac_servidor", 1, LOG_LEVEL_INFO);

    /*SERVER*/
    log_info(logger, "iniciando thread server");
    server_info* serverInfo = malloc(sizeof(server_info));
    serverInfo->logger = logger;
    serverInfo->portNumber = PORT;
    pthread_t tid;
    pthread_create(&tid, NULL, run_server, (void*) serverInfo);

    //run_server();
    //JOIN THREADS
    pthread_join(tid,NULL);
   
    //FREE MEMORY
    free(LOGPATH);
    free(logger);
    free(serverInfo);
    config_destroy(config);
   
 
    return 0;
}

/* action_open abre un archivo */
char* action_open(package_open* package) {
    log_info(logger,"Se recibió una accion open");
    return "je";
}

/* action_read leer un archivo abierto */
char* action_read(package_read* package) {
    log_info(logger,"Se recibio una accion read");
    return "ja";
}

/* action_getattr obtiene los atributos de un archivo */
char* action_getattr(package_getattr* package) {
    log_info(logger,"Se recibio una accion getattr");
    
    printf("El path recibido es: %s\n", package->path);

    if (strcmp(package->path, "/") == 0) {
        //devolver lo que corresponda al directorio raiz 
        log_info(logger, "el archivo encontrado es el directorio raiz");
        char* response = "S_IFDIR";
        return response;   
    } 

    //buscar el archivo
    int file_node_number;
    file_node_number = search_for_file(package->path);
    //ver que info saco de ahi? que luego voy a cargar en sac-cli stbuf

    if(file_node_number == 0) { //no existe
        //tengo que devolver -ENOENT
        char* response = "-ENOENT";
        return response;
    } else {
        //Si existe y es un directorio stbuff.st_mode = S_IFDIR | 0755
        //Si existe y es un archivo comun stbuff.st_mode = S_IFREG | 0777 y el stbuff.st_size

        //chequeo si es un directorio o un archivo regular
        FILE* disco = fopen("disco.bin", "r+");

        int offset = file_node_number * BLOCK_SIZE;
        fseek(disco, offset, SEEK_SET);

        struct sac_file_t file_node;

        fread(&file_node, sizeof(struct sac_file_t), 1, disco);

        if(file_node.state == 0 || file_node.state == 1) {
            //entonces es un archivo regular
            log_info(logger, "el archivo encontrado es un archivo regular (puede estar borrado)");
            int file_size = file_node.filesize;
            
            char* file_size_s; 
            sprintf(file_size_s,"%d",file_size);
            char* response = malloc(strlen("S_IFDIR")+strlen(file_size_s)+1);
            response[0] = '\0';
            strcat(response ,"S_IFDIR ");
            strcat(response ,file_size_s);

            close(disco);
            return response;
        } else if (file_node.state == 2) {
            //entonces es un directorio
            log_info(logger, "el archivo encontrado es un directorio");
            char* response = "S_IFDIR";
            close(disco);
            return response;
        } else {
            log_error(logger, "el estado del archivo encontrado es incorrecto");
            close(disco);
            char* response = "-ENOENT";
            return response;
        }
    }
}

/* action_mknod crea el nodo de un archivo */
char* action_mknod(package_mknod* package) {
    log_info(logger,"Se recibio una accion mknod");
    return "jo";
}

/* action_mkdir crea un directorio */
char* action_mkdir(package_mkdir* package) {
    log_info(logger,"Se recibio una accion mkdir");
    return "jo";
}

/* action_write escribe en un archivo abierto */
char* action_write(package_write* package) {
    log_info(logger,"Se recibio una accion write");
    return "jo";
}

/* action_opendir abre un directorio */
char* action_opendir(package_opendir* package) {
    log_info(logger,"Se recibio una accion opendir");
    return "jo";
}

/* action_readdir abre un directorio */
char* action_readdir(package_readdir* package) {
    log_info(logger,"Se recibio una accion readdir");
    return "jo";
}

int search_for_file(char* path){

    printf("se va a buscar el siguiente path: %s\n", path);

    //parsear el path y armar un array ej: /MiPc/Usuario/MisFotos/hola.jpg ->  [MiPc,Usuario,misFotos,hola.jpg]
    
    //Si el path es "/" (root directory), este existe pero no tiene un nodo asignado?
    //TODO ver que devolver en este caso
    if (strcmp(path,"/") == 0) {
        return 1;
    }


    char** path_array = parse_path(path);

    //Si el path es "/" (root directory), este existe pero no tiene un nodo asignado?
    //TODO ver que devolver en este caso
    /*if(path_array[0]==NULL){
        printf("path level 0 is null waching\n");
        free(path_array);
        return 1;
    }*/
    /*
    int path_array_length = 0;

    while (path_array[path_array_length] != NULL){
        
        printf("el nivel %d del path array es: %s\n", path_array_length, path_array[path_array_length]);
        
        path_array_length++;
    }*/

    //recorrer todos los nodos buscando el nombre del archivo
    int node = find_node(path_array);

    return node;

}

char** parse_path(char* path){

    printf("A parsear el path: %s \n", path);

    char** path_array = string_split(path, "/");

    if(path_array[0]==NULL){
        printf("path level 0 is null\n");
        free(path_array);
        return strdup("");
    } 

    //int instruction_size = strlen( parameters[0])+1;
    string_to_upper(path_array[0]);

    int path_array_length = 0;

    void kill_args(){
        for(int i =0; i < path_array_length; i++){
            free(path_array[i]);
        }
        free(path_array);
    }

    while (path_array[path_array_length] != NULL){
         
         if(path_array[path_array_length][0] == '"'){
            int value_length = strlen(path_array[path_array_length]);
            if(path_array[path_array_length][value_length-1]=='"'){
                char* value = malloc(strlen(path_array[path_array_length])-1);
                memcpy(value, path_array[path_array_length]+1, value_length-2);
                memcpy(value+value_length-2, "\0", 1);
                free(path_array[path_array_length]);
                path_array[path_array_length] = value;
            }else{
                path_array_length++;
                kill_args();
                //exec_err_abort(); TODO ver que hace y en que biblioteca esta
                return strdup("Parametro malformado.\n");
            }
        }
        path_array_length++;
    }

    printf("el nivel 0 del path es: %s\n", path_array[0]);

    return path_array;

}

int find_node(char** path_array) {

    struct sac_file_t current_node;

    int posible_matches[1024];
    int p_matches_position = 0;

    int path_array_length = 0;

    while (path_array[path_array_length] != NULL){
        path_array_length++;
    }

    FILE* disco = fopen("disco.bin", "r+");

    int offset = 2 * BLOCK_SIZE;
    fseek(disco, offset, SEEK_SET); //hacer que arranque en el ppio del bloque 2 
    //la tabla de nodos arranca en el bloque 2 de disco.bin y tiene 1024 bloques (va del 2 al 1025)
    //abro el disco y leo desde el primer nodo de la tabla de nodos hasta el último de la tabla de nodos

    for(int i=0; i<=1023; i++) {
        fread(&current_node, sizeof(struct sac_file_t), 1, disco);

        //comparo el ultimo string de path_array con el nombre del archivo
        if(strcmp(current_node.fname, path_array[path_array_length - 1]) == 0) {
            //como es igual agrego el numero de bloque a posible_matches
            int block_number = 2 + i;
            posible_matches[p_matches_position] = block_number;
            p_matches_position++;
        }
    }

    if(p_matches_position == 0) { //no hay coincidencias
        log_info(logger, "no se encontraron nodos con ese nombre");
        fclose(disco);
        return 0;
    } /*else if(p_matches_position == 1 && path_array_length == 1) {
        log_info(logger, "se encontro un nodo en el directorio raiz");
        fclose(disco);
        return posible_matches[p_matches_position];
    }*/

    //una vez que tengo el array con todos los resultados voy buscando uno por uno a ver si sigue el path ok
    //el largo real de posible matches es igual a p_matches_position

    int flag_found = 1;
    int flag_discarded = 1;
    int matches_count = p_matches_position;
    int current_match = matches_count - 1; //posicion del match en el array
    int match_found;
    //mientras no lo haya encontrado, si hay matches los evaluo
    while(flag_found == 1 && matches_count > 0) {

        //para un match en particular:
        struct sac_file_t match_node;
        int node_position = posible_matches[current_match] * BLOCK_SIZE;
        fseek(disco, node_position , SEEK_SET);
        fread(&match_node, sizeof(struct sac_file_t), 1, disco);
        
        //cargo el valor del primer padre en una estructura
        int parent = match_node.parent_block;
        
        int iterations = 1;
        int path_level = path_array_length - 1;
        //mientras no lo haya encontrado ni descartado:
        while(flag_found == 1 && flag_discarded == 1){

            //si el padre es 0: -> si el tamaño del path es igual a las iteraciones ? lo encontre : descarto el match
            if(parent == 0){
                if(path_array_length == iterations){
                    flag_found = 0;
                    match_found = current_match;
                } else {
                    flag_discarded = 0;
                }
            } else {
            //si el padre no es 0: -> si el tamaño del path es mayor a las iteraciones ? sigo : descarto el match
                struct sac_file_t parent_node;
                int parent_node_position = parent * BLOCK_SIZE;
                fseek(disco, parent_node_position , SEEK_SET);
                fread(&parent_node, sizeof(struct sac_file_t), 1, disco);

                if(path_array_length > iterations) {
                    //sigo -> el nombre del padre no coinside con el nivel del path correspondiente ? descarto el match : sigo
                    if(strcmp(parent_node.fname, path_array[path_level-iterations])!= 0){
                        flag_discarded = 0;
                    }
                } else {
                    flag_discarded = 0;
                }

                parent = parent_node.parent_block;
            }

            iterations++;

            
        }

        matches_count--;
        current_match--;

    }
    //si lo encontre retorno el match, sino veo que retornar

    fclose(disco);

    if(flag_found == 0) {
        //encontre el nodo
        return match_found;
    } else {
        //no encontre el archivo
        return 0;
    }

}


#include "sac_servidor.h"

t_log* logger;

struct sac_opened_file* opened_files_table_p = NULL;

int main(void) {
    printf("Corriendo el servidor\n");

    //check_disc();
    //dump_block(0);//header
    //dump_block(1);//bitmap
    //dump_block(2);//1er bloque de la tabla de nodos        
    //dump_block(2);
    //format_bitmap_nodetable();
    //dump_file_node(2);

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

    printf("El path recibido es: %s\n", package->path);

    if (strcmp(package->path, "/") == 0) {
        //es el directorio raiz 
        log_info(logger, "el archivo encontrado es el directorio raiz");
        char* response = "EISDIR";
        return response; 
    } 

    //buscar el archivo
    int file_node_number;
    file_node_number = search_for_file(package->path);

    if(file_node_number == 0) { //no existe
        char* response = "-ENOENT";
        return response;
    } else {
        //existe
        //chequear que sea un archivo
        int file_state = check_node_state(file_node_number);

        if(file_state == 0) {
            //el archivo fue borrado
            char* response = "-ENOENT";
            return response;
        } else if( file_state == 2) {
            //el archivo es un directorio
            char* response = "EISDIR";
            return response;
        }

        //cargar la info de la tabla de nodos en alguna estructura temporal con los datos del archivo
        //y el modo en el que se abrio
        FILE* disco = fopen("disco.bin", "r+");

        int offset = file_node_number * BLOCK_SIZE;
        fseek(disco, offset, SEEK_SET);

        struct sac_file_t file_node;

        fread(&file_node, sizeof(struct sac_file_t), 1, disco);
        fclose(disco);


        struct sac_opened_file* opened_file;
        opened_file = (struct sac_opened_file*)malloc(sizeof(struct sac_opened_file));

        //datos a cargar:
        opened_file->mode = package->flags;
        opened_file->state = file_state;
        memcpy(opened_file->fname, file_node.fname, strlen(file_node.fname));
        //opened_file->fname = file_node.fname;
        opened_file->parent_block = file_node.parent_block;

        int file_descriptor = insert_to_opened_files_table(opened_file);

        //convertir file_descriptor en char
        char* file_descriptor_s = malloc(sizeof(char) * 5);
        sprintf(file_descriptor_s,"%d",file_descriptor);

        return file_descriptor_s;
    }

    //retornar un int que represente el file descriptor
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
    	log_debug(logger, "file node number es cero");
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

        fread(&file_node, BLOCK_SIZE, 1, disco);

        if(file_node.state == (uint8_t) 0 || file_node.state == (uint8_t) 1) {
            //entonces es un archivo regular
            log_info(logger, "el archivo encontrado es un archivo regular (puede estar borrado)");
            int file_size = file_node.filesize;
            
            char file_size_s[5];
            sprintf(file_size_s,"%d",file_size);
            char* response = malloc(strlen("S_IFREG")+strlen(file_size_s)+2);
            response[0] = '\0';
            strcat(response ,"S_IFREG ");
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

    //buscar si no existe ya ese archivo -> si existe ver que retornar, si no existe sigo
    //buscar el archivo
    int file_node_number;
    file_node_number = search_for_file(package->path);

    printf("checkpoint 1\n");

    //si existe pero es un archivo directorio puedo crear un archivo regular con el mismo nombre con el mismo nombre
    int file_state;

    printf("checkpoint 2\n");

    if(file_node_number != 0) {

        file_state = check_node_state(file_node_number);

        if(file_state == 1 || file_state == 0){
            log_info(logger, "el archivo que quiere crear ya existe");
            char* response = "EEXIST";
            return response;
        }
    }
    
    printf("checkpoint 3\n");
    //buscar si existe el directorio donde quiero crearlo -> si no existe retornar el
    // error correspondiente, si existe guardar el nro de bloque de este directorio (ver que
    // hacer cuando este es 0) Tambien confirmar que sea un directorio.
    
    int directory_node_number;

    //obtener el path del directorio nomas
    char** path_array = string_split(package->path, "/");

    printf("checkpoint 4\n");

    int path_array_length = 0;
    while (path_array[path_array_length] != NULL){
        path_array_length++;
    }

    printf("checkpoint 5\n");
    int path_file_name_length = strlen(path_array[path_array_length - 1]);

    //char* file_name = path_array[path_array_length - 1];
    char* file_name = malloc(sizeof(char) * path_file_name_length + 1);
    file_name[0] = '\0';
    strcat(file_name , path_array[path_array_length - 1]);

    printf("checkpoint 6\n");

    int file_name_length = strlen(file_name);

    //control del largo del filename
    if(file_name_length > MAX_FILENAME_LENGTH) {
        log_info(logger, "el largo del nombre del archivo es mayor al permitido");
        char* response = "ENAMETOOLONG";
        return response;
    }

    printf("checkpoint 7\n");
    
    if(path_array_length == 1) {
        //el directorio es el directorio raiz
        directory_node_number = 0;
    } else {
        //crear path auxiliar para buscar el directorio

        printf("checkpoint 8\n");

        char* aux_path = malloc(strlen(package->path) - file_name_length + 1);
        aux_path[0] = '\0';
        strcat(aux_path ,"/");

        printf("checkpoint 9\n");

        int aux_path_length = path_array_length - 1;
        for(int i = 0; i < aux_path_length; i++) {
            strcat(aux_path ,path_array[i]);
            strcat(aux_path ,"/");
        }

        printf("El aux_path (path del directorio) es: %s\n", aux_path);

        directory_node_number = search_for_file(aux_path);

        if(directory_node_number == 0) {
            log_info(logger, "el directorio donde quiere crear el archivo no existe");
            char* response = "ENOENT";
            return response;
        }

        printf("checkpoint 10\n");

        //el nodo del directorio existe pero hay que comprobar que sea un directorio
        int directory_state = check_node_state(directory_node_number);

        if(directory_state != 2) {
            log_info(logger, "el directorio donde quiere crear el archivo no es un directorio");
            char* response = "ENOTDIR";
            return response;
        }
    }

    printf("checkpoint 11\n");
    //buscar un bloque libre (dentro de la tabla de nodos)
    int free_block = find_free_block(0); // 0 -> node table

    dump_block(1);

    printf("checkpoint 12 el free block es el: %d\n", free_block);

    //cambio el bitmap para que lo muestre como ocupado
    set_block_as_occupied(free_block);

    dump_block(1);

    printf("checkpoint 13\n");

    //cargar un nodo con los datos del nuevo archivo y escribirlo en la tabla de nodos
    struct sac_file_t file_node;
    file_node.state = 1; //ocupado
    memcpy(file_node.fname, file_name, file_name_length);
    //file_node.fname = file_name;
    file_node.parent_block = directory_node_number;
    file_node.filesize = 0;
    file_node.created = time(NULL);
    file_node.modified = time(NULL);
    file_node.blocks[0] = '\0';

    printf("checkpoint 14\n");

    //hacer fopen fwrite y fclose del disco
    FILE* disco = fopen("disco.bin", "r+");

    int offset = BLOCK_SIZE * free_block;
    fseek(disco, offset, SEEK_SET); //en la posicion del bloque libre

    fwrite(&file_node, BLOCK_SIZE, 1, disco);

    fclose(disco);

    printf("checkpoint 15\n");

    char* response = malloc(sizeof(char)*2);
    //response[0] = '\0';
    sprintf(response,"%d",free_block);

    return response;
}

/* action_mkdir crea un directorio */
char* action_mkdir(package_mkdir* package) {
    log_info(logger,"Se recibio una accion mkdir");
    
    //buscar el archivo
    int file_node_number;
    file_node_number = search_for_file(package->path);

    //TODO si existe pero es un archivo regular puedo crear un directorio con el mismo nombre
    int file_state;

    if(file_node_number != 0) {

        file_state; = check_node_state(file_node_number);

        if(file_state == 2) {
            log_info(logger, "el directorio que quiere crear ya existe");
            char* response = "EEXIST";
            return response;
        }
    }
    
    //buscar si existe el directorio donde quiero crearlo -> si no existe retornar el
    // error correspondiente, si existe guardar el nro de bloque de este directorio (ver que
    // hacer cuando este es 0) Tambien confirmar que sea un directorio.
    
    int directory_node_number;

    //obtener el path del directorio nomas
    char** path_array = string_split(package->path, "/");

    int path_array_length = 0;
    while (path_array[path_array_length] != NULL){
        path_array_length++;
    }

    int path_file_name_length = strlen(path_array[path_array_length - 1]);

    //char* file_name = path_array[path_array_length - 1];
    char* file_name = malloc(sizeof(char) * path_file_name_length + 1);
    file_name[0] = '\0';
    strcat(file_name , path_array[path_array_length - 1]);

    int file_name_length = strlen(file_name);

    //control del largo del filename
    if(file_name_length > MAX_FILENAME_LENGTH) {
        log_info(logger, "el largo del nombre del directorio es mayor al permitido");
        char* response = "ENAMETOOLONG";
        return response;
    }
    
    if(path_array_length == 1) {
        //el directorio es el directorio raiz
        directory_node_number = 0;
    } else {
        //crear path auxiliar para buscar el directorio

        char* aux_path = malloc(strlen(package->path) - file_name_length + 1);
        aux_path[0] = '\0';
        strcat(aux_path ,"/");

        int aux_path_length = path_array_length - 1;
        for(int i = 0; i < aux_path_length; i++) {
            strcat(aux_path ,path_array[i]);
            strcat(aux_path ,"/");
        }

        printf("El aux_path (path del directorio) es: %s\n", aux_path);

        directory_node_number = search_for_file(aux_path);

        if(directory_node_number == 0) {
            log_info(logger, "el directorio donde quiere crear el directorio no existe");
            char* response = "ENOENT";
            return response;
        }

        //el nodo del directorio existe pero hay que comprobar que sea un directorio
        int directory_state = check_node_state(directory_node_number);

        if(directory_state != 2) {
            log_info(logger, "el directorio donde quiere crear el directorio no es un directorio");
            char* response = "ENOTDIR";
            return response;
        }
    }

    //buscar un bloque libre (dentro de la tabla de nodos)
    int free_block = find_free_block(0); // 0 -> node table

    //cambio el bitmap para que lo muestre como ocupado
    set_block_as_occupied(free_block);

    //cargar un nodo con los datos del nuevo archivo y escribirlo en la tabla de nodos
    struct sac_file_t file_node;
    file_node.state = 2; //directorio
    memcpy(file_node.fname, file_name, file_name_length);
    //file_node.fname = file_name;
    file_node.parent_block = directory_node_number;
    file_node.filesize = 0;
    file_node.created = time(NULL);
    file_node.modified = time(NULL);
    file_node.blocks[0] = '\0';

    //hacer fopen fwrite y fclose del disco
    FILE* disco = fopen("disco.bin", "r+");

    int offset = BLOCK_SIZE * free_block;
    fseek(disco, offset, SEEK_SET); //en la posicion del bloque libre

    fwrite(&file_node, BLOCK_SIZE, 1, disco);

    fclose(disco);

    char* response = malloc(sizeof(char)*2);
    sprintf(response,"%d",free_block);

    return response;
}

/* action_write escribe en un archivo abierto */
char* action_write(package_write* package) {
    log_info(logger,"Se recibio una accion write");
    return "jo";
}

/* action_opendir abre un directorio */
char* action_opendir(package_opendir* package) {
    log_info(logger,"Se recibio una accion opendir");
    
    printf("El path recibido es: %s\n", package->path);

    if (strcmp(package->path, "/") == 0) {
        //TODO es el directorio raiz ver que hacer
        log_info(logger, "el archivo encontrado es el directorio raiz");
        char* response = "EISDIR";
        return response; 
    } 

    //buscar el archivo
    int file_node_number;
    file_node_number = search_for_file(package->path);

    if(file_node_number == 0) { //no existe
        char* response = "-ENOENT";
        return response;
    } else {
        //existe
        //chequear que sea un archivo
        int file_state = check_node_state(file_node_number);

        if(file_state == 0) {
            //el archivo fue borrado
            char* response = "-ENOENT";
            return response;
        } else if( file_state == 1) {
            //TODO el directorio es un archivo ver que hacer
            char* response = "EISDIR";
            return response;
        }

        //cargar la info de la tabla de nodos en alguna estructura temporal con los datos del archivo
        //y el modo en el que se abrio
        FILE* disco = fopen("disco.bin", "r+");

        int offset = file_node_number * BLOCK_SIZE;
        fseek(disco, offset, SEEK_SET);

        struct sac_file_t file_node;

        fread(&file_node, sizeof(struct sac_file_t), 1, disco);
        fclose(disco);


        struct sac_opened_file* opened_file;
        opened_file = (struct sac_opened_file*)malloc(sizeof(struct sac_opened_file));

        //datos a cargar:
        opened_file->mode = package->flags;
        opened_file->state = file_state;
        memcpy(opened_file->fname, file_node.fname, strlen(file_node.fname));
        //opened_file->fname = file_node.fname;
        opened_file->parent_block = file_node.parent_block;

        int file_descriptor = insert_to_opened_files_table(opened_file);

        //convertir file_descriptor en char
        char* file_descriptor_s = malloc(sizeof(char) * 5);
        sprintf(file_descriptor_s,"%d",file_descriptor);

        return file_descriptor_s;
    }

    //retornar un int que represente el file descriptor
    
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
    //string_to_upper(path_array[0]);

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
        fread(&current_node, BLOCK_SIZE, 1, disco);

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
        return posible_matches[match_found];
    } else {
        //no encontre el archivo
        return 0;
    }

}

int check_node_state(int directory_node_number) {

    FILE* disco = fopen("disco.bin", "r+");

    int offset = directory_node_number * BLOCK_SIZE;
    fseek(disco, offset, SEEK_SET);

    struct sac_file_t check_node;
    fread(&check_node, sizeof(struct sac_file_t), 1, disco);

    int state = check_node.state;

    close(disco);

    return state;
}

int find_free_block(int area) {

    printf("checkpoint 11 a\n");

    //defino donde esta el bitmap (es el segundo bloque)
    int bitmap_offset = BLOCK_SIZE;
    //TODO ver que el bitmap esta definido en el header del fs -> tendria que leerlo de ahi calcular donde esta y seguir

    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir
    //leer el bitmap
    fseek(disco, bitmap_offset, SEEK_SET);

    char* bitmap = malloc(BLOCK_SIZE + 1);
    fread(bitmap ,sizeof(char) ,BLOCK_SIZE ,disco); //TODO si cambia el tamaño del bitmap tengo que reemplazar BLOCK_SIZE por el tamaño del bitmap
    fclose(disco);

    printf("checkpoint 11 b\n");

    //printf("el bitmap es:\n %s\n", bitmap);
    
    //defino segun el place el area del bitmap donde voy a buscar (la parte que corresponde a la tabla de 
    //nodos o la parte que corresponde a los bloques de datos)
    //TODO si los tamaños varian ver como ajustar
    int area_start;
    int area_end;

    if(area == 0) {
        //busco en la tabla de nodos
        area_start = 2;
        area_end = 1025;
    } else if (area == 1) {
        //busco en los bloques de datos
        area_start = 1026;
        area_end = 32767;
    }

    printf("checkpoint 11 c\n");

    t_bitarray* bitarray = bitarray_create_with_mode(bitmap, 4096, LSB_FIRST);
    
    //recorro el bitmap en esa seccion buscando un 0, si lo encuentro devuelvo su posición, sino devuelvo 0
    for(int i = area_start; i<=area_end ;i++) {
        //printf("checkpoint 11 c for loop\n");
        

        if(bitarray_test_bit(bitarray, i) == 0){ //recorro todo el bitmap buscando un 0 if(bitmap[i]=='0')
            printf("checkpoint 11 c iteration number %d\n", i);
            
            free(bitmap);
            return i; //devuelvo el indice del primer bloque libre que encuentro -> es el numero de bloque libre
        }
    }

    printf("checkpoint 11 d\n");    

    //si no encontró un bloque libre
    free(bitmap);
    log_error(logger,"No se encontraron bloques libres");
    return -1;
}

void set_block_as_occupied(int block_number) {
    
    //defino donde esta el bitmap (es el segundo bloque)
    int bitmap_offset = BLOCK_SIZE;
    //TODO ver que el bitmap esta definido en el header del fs -> tendria que leerlo de ahi calcular donde esta y seguir

    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir
    //leer el bitmap
    fseek(disco, bitmap_offset, SEEK_SET);

    char* bitmap = malloc(BLOCK_SIZE + 1);
    fread(bitmap, BLOCK_SIZE, 1, disco); //TODO si cambia el tamaño del bitmap tengo que reemplazar BLOCK_SIZE por el tamaño del bitmap


    printf("EL BITMAP ES :\n");

    int row = 1;
    int end = strlen(bitmap);
    for(int k=0; k<end; k=k+8 ) {

        printf("ROW %d - : ", row);

        chartobin(bitmap[k]);
        chartobin(bitmap[k+1]);
        chartobin(bitmap[k+2]);
        chartobin(bitmap[k+3]);
        chartobin(bitmap[k+4]);
        chartobin(bitmap[k+5]);
        chartobin(bitmap[k+6]);
        chartobin(bitmap[k+7]);
        

        printf("\n");

        row++;

    }

    t_bitarray* bitarray = bitarray_create_with_mode(bitmap, 4096, LSB_FIRST);

    //bitmap[block_number] = '1';

    bitarray_set_bit(bitarray, block_number);

    printf("//////////////////setting bitearray////////////////////////\n");
    int rowb = 1;
    int endb = strlen(bitarray->bitarray);
    for(int k=0; k<endb; k=k+8 ) {

        printf("ROW %d - : ", rowb);

        chartobin(bitarray->bitarray[k]);
        chartobin(bitarray->bitarray[k+1]);
        chartobin(bitarray->bitarray[k+2]);
        chartobin(bitarray->bitarray[k+3]);
        chartobin(bitarray->bitarray[k+4]);
        chartobin(bitarray->bitarray[k+5]);
        chartobin(bitarray->bitarray[k+6]);
        chartobin(bitarray->bitarray[k+7]);
        

        printf("\n");

        rowb++;

    }
    printf("///////////////////////////////////////////////////////////\n");

    //bitmap = bitarray->bitarray;

    fseek(disco, bitmap_offset, SEEK_SET);

    fwrite(bitarray->bitarray, BLOCK_SIZE, 1, disco); //TODO ver de estar haciendo bien el read write
    fclose(disco);
    free(bitmap);
    return;
}

int insert_to_opened_files_table( struct sac_opened_file* opened_file){

    int fd = 1;

    if (opened_files_table_p == NULL) { //la tabla de archivos abiertos esta vacia - agrego archivo abierto nuevo
        
        printf("La tabla de archivos abiertos está vacia\n");
        
        struct sac_opened_file* new_nodo_file;
        new_nodo_file = (struct sac_opened_file*)malloc(sizeof(struct sac_opened_file));

        //datos a cargar:
        new_nodo_file->fd = fd;
        new_nodo_file->mode = opened_file->mode;
        new_nodo_file->state = opened_file->state;
        //strncpy(new_nodo_file->fname, opened_file->fname, MAX_FILENAME_LENGTH);
        //new_nodo_file->fname[MAX_FILENAME_LENGTH - 1] = '\0';
        memcpy(new_nodo_file->fname, opened_file->fname, MAX_FILENAME_LENGTH);
        //new_nodo_file->fname = opened_file->fname;
        new_nodo_file->parent_block = opened_file->parent_block;
        new_nodo_file->next = NULL;

        //agrego el archivo a la tabla de archivos abiertos
        opened_files_table_p = new_nodo_file;

    } else { //la tabla de archivos abiertos no esta vacia

        printf("La tabla de archivos abiertos no esta vacia\n");

        struct sac_opened_file* aux_node;
        aux_node = opened_files_table_p;

        while(aux_node->next != NULL) { //avanzo hasta el final
            
            aux_node = aux_node->next;

        }

        fd = aux_node->fd + 1;

        //agrego el archivo abierto a la tabla
        struct sac_opened_file* new_nodo_file;
        new_nodo_file = (struct sac_opened_file*)malloc(sizeof(struct sac_opened_file));

        //datos a cargar:
        new_nodo_file->fd = fd;
        new_nodo_file->mode = opened_file->mode;
        new_nodo_file->state = opened_file->state;
        memcpy(new_nodo_file->fname, opened_file->fname, strlen(opened_file->fname));
        //new_nodo_file->fname = opened_file->fname;
        new_nodo_file->parent_block = opened_file->parent_block;
        new_nodo_file->next = NULL;

        //agrego el archivo a la tabla de archivos abiertos
        aux_node->next = new_nodo_file;

    }

    return fd;
}


///////////////////////////////////////////////////////////////////

/*
void set_block_as_free(int block_number) {
         
    FILE* bitmap_file = fopen(bitmap_path,"r+");
    char *bitmap = malloc(block_amount+2);
    fread(bitmap, sizeof(char),block_amount, bitmap_file);
    fseek(bitmap_file, 0, SEEK_SET);
    bitmap[block_number] = '0';
    fwrite(bitmap, sizeof(char), block_amount, bitmap_file);
    fclose(bitmap_file);
    free(bitmap);
    return;
}
*/

/*void check_disc() {
    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir
    fseek(disco, 0, SEEK_SET);

    struct sac_header_t header;

    fread(&header,BLOCK_SIZE, 1, disco);

    printf("el header id es: %s\n",header.id);
    printf("la version es : %d", header.version);
    printf("el bitmap start es : %d", header.bitmap_start);
    printf("el bitmap size es : %d", header.bitmap_size);

    fclose(disco);
    
}*/

void dump_block(int b) {
    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir

    int offset = b * BLOCK_SIZE;

    fseek(disco, offset, SEEK_SET);

    struct sac_block_t bitmap_block;

    int end = BLOCK_SIZE - 1;

    fread(&bitmap_block, BLOCK_SIZE, 1, disco);

    int row = 1;

    for(int k=0; k<end; k=k+8 ) {

        printf("ROW %d - : ", row);

        chartobin(bitmap_block.bytes[k]);
        chartobin(bitmap_block.bytes[k+1]);
        chartobin(bitmap_block.bytes[k+2]);
        chartobin(bitmap_block.bytes[k+3]);
        chartobin(bitmap_block.bytes[k+4]);
        chartobin(bitmap_block.bytes[k+5]);
        chartobin(bitmap_block.bytes[k+6]);
        chartobin(bitmap_block.bytes[k+7]);
        

        printf("\n");

        row++;

    }


    fclose(disco);
}

void chartobin(char c) {
    int i;
    for(i=0; i<=7; i++){
        putchar((c & (1 << i)) ? '1' : '0');
    }
    printf(" ");
}

void format_bitmap_nodetable() {
    
    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir

    int offset = 1 * BLOCK_SIZE;

    fseek(disco, offset, SEEK_SET);

    struct sac_block_t bitmap_block;

    fread(&bitmap_block, BLOCK_SIZE, 1, disco);

    t_bitarray* bitarray = bitarray_create_with_mode(bitmap_block.bytes, 4096, LSB_FIRST);

    int area_start = 2;
    int area_end = 4096;

    for(int i = area_start; i<=area_end ;i++) { 
        if(bitarray_test_bit(bitarray, i) == 1){ 
            bitarray_clean_bit(bitarray, i);
        }
    }

    //escribo
    fseek(disco, offset, SEEK_SET);
    
    fwrite(bitarray->bitarray, BLOCK_SIZE, 1, disco); //TODO ver de estar haciendo bien el read write
    fclose(disco);

    dump_block(1);
    
    return;
}

void dump_file_node(int block_number) {

    FILE* disco = fopen("disco.bin", "r+"); //TODO ver con que permisos lo tengo que abrir

    int offset = block_number * BLOCK_SIZE;

    fseek(disco, offset, SEEK_SET);

    struct sac_file_t file_node;

    fread(&file_node, BLOCK_SIZE, 1, disco);

    printf("\n\n");
    printf("Dumpeando el bloque nro %d de la tabla de nodos\n", block_number);
    printf("El estado es: %d\n", file_node.state);
    printf("El nombre es: %s\n", file_node.fname);
    printf("El padre es: %d\n", file_node.parent_block);
    printf("El tamaño es: %d\n", file_node.filesize);
    printf("El creado es: %d\n", file_node.created);
    printf("El modificado es: %d\n", file_node.modified);

    return;

}

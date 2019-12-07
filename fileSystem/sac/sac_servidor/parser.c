#include "parser.h"

char* parse_input(char* instr_buff){ //aca da error

    printf("instruccion:%s\n",instr_buff); //La dejo para debug

    char buff[strlen(instr_buff)+2];
    memcpy(buff, instr_buff, strlen(instr_buff)+1);


    int i = 0;
    int offset = 0;
    bool last_character_was_null = false;
    bool openText = false;
    while(buff[i+offset] !='\0'){
        if(!openText){
            if(buff[i]== '\n' || buff[i]==' '){

            buff[i]= '|';
            buff[i] = '|';

            if(last_character_was_null){
                //int len = strlen(buff)+1;
                char* tmp = string_substring_from(buff,i+1);
                memcpy(&buff[i], tmp, strlen(tmp)+1);
                free(tmp);
                i--;
            }

            last_character_was_null = true;

            }else{

                buff[i] = buff[i];   
                last_character_was_null = false;
                if(buff[i]=='"') openText = true;
            }
        }else{
            if(buff[i] == '"') openText = false;
        }

    i++;        
    //quito las nuevas lineas 
    //quito los espacios 
    }
    buff[i-offset+1] = '\0';

    char** parameters = string_split(buff, "|");

    if(parameters[0]==NULL){
        printf("parameter 0 is null\n");
        free(parameters);
        return strdup("");
    } 

    //int instruction_size = strlen( parameters[0])+1;
    string_to_upper(parameters[0]);

    int parameters_length = 0;

    void kill_args(){
        for(int i =0; i < parameters_length; i++){
            free(parameters[i]);
        }
        free(parameters);
    }

    while (parameters[parameters_length] != NULL){
         
         if(parameters[parameters_length][0] == '"'){
            int value_length = strlen(parameters[parameters_length]);
            if(parameters[parameters_length][value_length-1]=='"'){
                char* value = malloc(strlen(parameters[parameters_length])-1);
                memcpy(value, parameters[parameters_length]+1, value_length-2);
                memcpy(value+value_length-2, "\0", 1);
                free(parameters[parameters_length]);
                parameters[parameters_length] = value;
            }else{
                parameters_length++;
                kill_args();
                //exec_err_abort(); TODO ver que hace y en que biblioteca esta
                return strdup("Parametro malformado.\n");
            }
        }
        parameters_length++;
    }

    printf("parameter 0 is: %s\n", parameters[0]);

    if(!strcmp(parameters[0],"OPEN")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_open* package = malloc(sizeof(package_open));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_open(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"READ")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_read* package = malloc(sizeof(package_read));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_read(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"GETATTR")){
        
        if(parameters_length != 2){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_getattr* package = malloc(sizeof(package_getattr));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        //package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_getattr(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"MKNOD")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_mknod* package = malloc(sizeof(package_mknod));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_mknod(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"MKDIR")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_mkdir* package = malloc(sizeof(package_mkdir));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_mkdir(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"WRITE")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_write* package = malloc(sizeof(package_write));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_write(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"OPENDIR")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_opendir* package = malloc(sizeof(package_opendir));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_opendir(package);
        free(parameters);
        return response;
    }

    if(!strcmp(parameters[0],"READDIR")){
        
        if(parameters_length != 3){
            printf("numero de parametros incorrecto\n");
            kill_args();
            //exec_err_abort(); TODO ver para que sirve y en que biblioteca esta
            return strdup("Numero de parametros incorrectos\n");
        } 

        //TODO revisar esta comprobacion de parametros
        /*if(strspn(parameters[2], "0123456789")!=strlen(parameters[2])){
            printf("comprobacion de parametros rara\n");
            kill_args();
            //exec_err_abort(); TODO ver que hace y en que bibloteca esta
            return strdup("Parametro mal formado\n");
        } */

        package_readdir* package = malloc(sizeof(package_readdir));
        package->instruction = parameters[0];

        //PATH
        //string_to_upper(parameters[1]);
        package->path = parameters[1];

        //MODE
        package->mode = parameters[2];

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* response = action_readdir(package);
        free(parameters);
        return response;
    }

    kill_args();
    //exec_err_abort(); TODO ver que hace y en que biblioteca esta
    char* error_message = strdup("No es una instruccion valida\n");
    return error_message;
    
}
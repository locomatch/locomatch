#include "sac_cli.h"

#define IP "127.0.0.1"
#define PUERTO "8003"
#define PACKAGESIZE 1024

int serverSocket;
t_log* logger;

/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE
 */
struct t_runtime_options {
	char* welcome_msg;
} runtime_options;

/*
 * Esta Macro sirve para definir nuestros propios parametros que queremos que
 * FUSE interprete. Esta va a ser utilizada mas abajo para completar el campos
 * welcome_msg de la variable runtime_options
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

/*
 * Esta es la estructura principal de FUSE con la cual nosotros le decimos a
 * biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
 * Como se observa la estructura contiene punteros a funciones.
 */
struct fuse_operations sac_oper = {
  .open = sac_open,
  .read = sac_read,
  .getattr = sac_getattr,
  .mknod = sac_mknod,
  .mkdir = sac_mkdir,
  .write = sac_write
  /*.opendir = sac_opendir,
  .readdir = sac_readdir*/
  //TODO ver operaciones faltantes
};

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};

/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

int main (int argc,char *argv[]) {
    printf("Corriendo el cliente\n");
    
    //acá tengo que implementar fuse para mandar todo a sac_servidor que se va a encargar de implementar el fs

    /*LOG*/
    char* LOGPATH = "sac_cli.log";
    logger = log_create(LOGPATH, "Sac_cli", 1, LOG_LEVEL_INFO);

    //run client
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP
    getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion
    //int serverSocket;
	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

    /*FUSE*/
    // See which version of fuse we're running
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

    // Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &sac_oper, NULL);

}

void sac_send(char* msg, int serverSocket){

    /*int enviar = 1;
	char message[PACKAGESIZE];

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

	while(enviar){
		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
		if (enviar) send(serverSocket, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
	}*/

    send(serverSocket, msg, strlen(msg) + 1, 0);
}

/*void sac_open(char* msg) {
    sac_send(msg, serverSocket);
}*/

/* sac_open abre un archivo */
static int sac_open(const char *path, struct fuse_file_info *fi) {
    log_info(logger,"Se recibio una instruccion open");

    //a sac_cli yo le voy a mandar el path y las flags de fi y me va a devolver un int que es el file descriptor

    int flags = fi->flags;
    char flags_s[10]; 
    sprintf(flags_s,"%d",flags);

    char* msg = malloc(strlen("open ")+strlen(path)+strlen(" ")+strlen(flags_s)+2);
    msg[0] = '\0';
    strcat(msg ,"open ");
    strcat(msg ,path);
    strcat(msg ," ");
    strcat(msg ,flags_s);

    log_info(logger, "El mensaje a enviar es: %s", msg);

    sac_send(msg, serverSocket);
    
    free(msg);

    //obtener el responce para cargar la estructura que tiene que devolver esta garompa
    char* response_buff = malloc(100);
    response_buff[0] = '\0';
    read(serverSocket, response_buff, 100);

    printf("el response_buff dice esto: %s\n", response_buff);

    if(strcmp(response_buff,"EISDIR") == 0) {
        //es un directorio
        log_info(logger, "El archivo que se intenta abrir es un directorio");
        errno = EISDIR;
        return -1;
    } else if (strcmp(response_buff,"-ENOENT") == 0) {
        //no existe o fue borrado
        log_info(logger, "El archivo que se intenta abrir no existe o fue borrado");
        errno = ENOENT;
        return -1;

        //TODO creo que tenia que setear errno y retornar -1   <-- VER ESTO

    } else {
        //retorno un string que contiene el file descriptor
        int file_descriptor = 0;

        if(response_buff[0] == '\0') {
        	file_descriptor = atoi(response_buff);
        }

        return file_descriptor;
    }
}

/* sac_read leer un archivo abierto */
int sac_read(char* msg) {
    log_info(logger,"Se recibio una instruccion read");
    sac_send(msg, serverSocket);
    return 0;
}

/* sac_getattr obtiene los atributos de un archivo */
static int sac_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    log_info(logger,"Se recibio una instruccion getattr");

    (void) fi;
    //int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    char* msg = malloc(strlen("getattr ")+strlen(path)+2);
    msg[0] = '\0';
    strcat(msg ,"getattr ");
    strcat(msg ,path);

    log_info(logger, "El mensaje a enviar es: %s", msg);

    sac_send(msg, serverSocket);
    free(msg);

    //obtener el responce para cargar la estructura que tiene que devolver esta garompa
    char* response_buff = malloc(100);
    response_buff[0] = '\0';
    read(serverSocket, response_buff, 100);

    printf("el response_buff dice esto: %s\n", response_buff);

    //cargar stbuf dependiendo de la respuesta
    if(strcmp(response_buff,"-ENOENT") == 0) {
        //retornar el error
        log_info(logger, "No se encontró el archivo");
        //errno = ENOENT;
        return -ENOENT;
    } else if (strcmp(response_buff,"S_IFDIR") == 0) {
        //es un directorio
        log_info(logger, "Se encontro un directorio");
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } else {
        //es un archivo regular (o hubo algun error al recibir la respuesta de sac_server a getattr)
        //parseo response_buff para que me quede un array con [S_IFREG,123223] pasar el segundo valor a numeros porque esta en char
        char** response_array = string_split(response_buff, " ");

        if(strcmp(response_array[0],"S_IFREG") == 0) {
            //es un archivo regular
            int file_size = atoi(response_array[1]);

            stbuf->st_mode = S_IFREG | 0777;
            stbuf->st_nlink = 1;
            stbuf->st_size = file_size;
            return 0;
        } else {
            //hubo un error
            log_error(logger, "Hubo algun error al regresar getattr");
            //errno = ENOENT;
            return -ENOENT;
        }
    }

    return 0;
}

/* sac_mknod crea el nodo de un archivo */
static int sac_mknod(char* path, mode_t mode, dev_t rdev) {

    //vamos a asumir que el archivo no tiene permisos, o mejor dicho que todos tienen permisos para todos los archivos
    //mode_t mode hace referencia a estos permisos asique no vamos a usarlo. (y al tipo de archivo que vamos a obviar)
    //dev_t rdev hace referencia al dispositivo que contiene el archivo? no vamos a usarlo tampoco

    log_info(logger,"Se recibio una instruccion mknod");

    char* msg = malloc(strlen("mknod ")+strlen(path)+2);
    msg[0] = '\0';
    strcat(msg ,"mknod ");
    strcat(msg ,path);

    log_info(logger, "El mensaje a enviar es: %s", msg);

    sac_send(msg, serverSocket);
    free(msg);

    //obtener el responce para cargar la estructura que tiene que devolver esta garompa
    char* response_buff = malloc(100);
    response_buff[0] = '\0';
    read(serverSocket, response_buff, 100);

    printf("el response_buff dice esto: %s\n", response_buff);

    if(strcmp(response_buff,"EEXIST") == 0) {
        //retornar el error
        log_info(logger, "el archivo que quiere crear ya existe");
        errno = EEXIST;
        return -1;
    } else if (strcmp(response_buff,"ENAMETOOLONG") == 0) {
        //retornar el error
        log_info(logger, "el nombre del archivo que quiere crear es demasiado largo");
        errno = ENAMETOOLONG;
        return -1;
    } else if (strcmp(response_buff,"ENOENT") == 0) {
        //retornar el error
        log_info(logger, "el directorio donde quiere crear el archivo no existe");
        errno = ENOENT;
        return -1;
    } else if (strcmp(response_buff,"ENOTDIR") == 0) {
        //retornar el error
        log_info(logger, "el directorio donde quiere crear el archivo no es un directorio");
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

/* sac_mkdir crea un directorio */
static int sac_mkdir(char* path, mode_t mode) {
    log_info(logger,"Se recibio una instruccion mkdir");

    char* msg = malloc(strlen("mkdir ")+strlen(path)+2);
    msg[0] = '\0';
    strcat(msg ,"mkdir ");
    strcat(msg ,path);

    log_info(logger, "El mensaje a enviar es: %s", msg);

    sac_send(msg, serverSocket);
    free(msg);

    //obtener el responce para cargar la estructura que tiene que devolver esta garompa
    char* response_buff = malloc(100);
    response_buff[0] = '\0';
    read(serverSocket, response_buff, 100);

    printf("el response_buff dice esto: %s\n", response_buff);

    if(strcmp(response_buff,"EEXIST") == 0) {
        //retornar el error
        log_info(logger, "el directorio que quiere crear ya existe");
        errno = EEXIST;
        return -1;
    } else if (strcmp(response_buff,"ENAMETOOLONG") == 0) {
        //retornar el error
        log_info(logger, "el nombre del directorio que quiere crear es demasiado largo");
        errno = ENAMETOOLONG;
        return -1;
    } else if (strcmp(response_buff,"ENOENT") == 0) {
        //retornar el error
        log_info(logger, "el directorio donde quiere crear el directorio no existe");
        errno = ENOENT;
        return -1;
    } else if (strcmp(response_buff,"ENOTDIR") == 0) {
        //retornar el error
        log_info(logger, "el directorio donde quiere crear el directorio no es un directorio");
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

/* sac_write escribe en un archivo abierto */
int sac_write(char* msg) {
    log_info(logger,"Se recibio una instruccion write");
    sac_send(msg, serverSocket);
    return 0;
}

/* sac_opendir abre un directorio */
int sac_opendir(char* msg) {
    log_info(logger,"Se recibio una instruccion opendir");
    sac_send(msg, serverSocket);
    return 0;
}

/* sac_readdir abre un directorio */
int sac_readdir(char* msg) {
    log_info(logger,"Se recibio una instruccion readdir");
    sac_send(msg, serverSocket);
    return 0;
}

/*
 * Cliente FTP Concurrente - YungaB-clienteFTP.c
 * Implementa un cliente FTP que soporta múltiples transferencias concurrentes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define CMD_SIZE 512

extern int connectTCP(const char* host, const char* service);
extern int errexit(const char* format, ...);

/* Variables globales */
int control_sock = -1;
char server_host[256];

int leer_respuesta(int sock, char* buffer, int buf_size) {
    int n, codigo;
    memset(buffer, 0, buf_size);

    n = recv(sock, buffer, buf_size - 1, 0);
    if (n <= 0) {
        fprintf(stderr, "Error leyendo respuesta del servidor\n");
        return -1;
    }

    buffer[n] = '\0';
    printf("S: %s", buffer);

    sscanf(buffer, "%d", &codigo);
    return codigo;
}

/*enviar_comando - Envía un comando al servidor FTP*/
int enviar_comando(int sock, const char* cmd) {
    char buffer[CMD_SIZE];
    int n;

    snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    printf("C: %s", buffer);

    n = send(sock, buffer, strlen(buffer), 0);
    if (n < 0) {
        fprintf(stderr, "Error enviando comando\n");
        return -1;
    }
    return 0;
}

/* cmd_user - Comando USER para autenticación*/
int cmd_user(const char* username) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE];
    int codigo;

    snprintf(cmd, sizeof(cmd), "USER %s", username);
    if (enviar_comando(control_sock, cmd) < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    return (codigo == 331 || codigo == 230) ? 0 : -1;
}

/* cmd_pass - Comando PASS para autenticación */
int cmd_pass(const char* password) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE];
    int codigo;

    snprintf(cmd, sizeof(cmd), "PASS %s", password);
    if (enviar_comando(control_sock, cmd) < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    return (codigo == 230) ? 0 : -1;
}

/* parsear_pasv - Extrae la IP y el puerto de la respuesta del modo pasivo. */
int parsear_pasv(const char* response, char* ip, int* puerto) {
    int h1, h2, h3, h4, p1, p2;
    char* inicio;

    inicio = strchr(response, '(');
    if (!inicio) return -1;

    if (sscanf(inicio, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6)
        return -1;

    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    *puerto = p1 * 256 + p2;

    return 0;
}

/* cmd_pasv - Inicia el modo pasivo y establece una conexión de datos. */
int cmd_pasv() {
    char response[BUFFER_SIZE], ip[64], puerto_str[16];
    int codigo, puerto, data_sock;

    if (enviar_comando(control_sock, "PASV") < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    if (codigo != 227) {
        fprintf(stderr, "Error en comando PASV\n");
        return -1;
    }

    if (parsear_pasv(response, ip, &puerto) < 0) {
        fprintf(stderr, "Error parseando respuesta PASV\n");
        return -1;
    }

    printf("Conectando a modo pasivo: %s:%d\n", ip, puerto);

    snprintf(puerto_str, sizeof(puerto_str), "%d", puerto);
    data_sock = connectTCP(ip, puerto_str);

    return data_sock;
}

/* cmd_retr - Descarga un archivo del servidor. */
int cmd_retr(const char* archivo_remoto, const char* archivo_local) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE], buffer[BUFFER_SIZE];
    int data_sock, codigo, n;
    FILE* fp;
    long total = 0;

    /* Entrar en modo pasivo */
    data_sock = cmd_pasv();
    if (data_sock < 0)
        return -1;

    /* Enviar comando RETR */
    snprintf(cmd, sizeof(cmd), "RETR %s", archivo_remoto);
    if (enviar_comando(control_sock, cmd) < 0) {
        close(data_sock);
        return -1;
    }

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    if (codigo != 150 && codigo != 125) {
        fprintf(stderr, "Error: servidor no inició transferencia\n");
        close(data_sock);
        return -1;
    }

    /* Abrir archivo local para escritura */
    fp = fopen(archivo_local, "wb");
    if (!fp) {
        fprintf(stderr, "Error abriendo archivo local: %s\n", archivo_local);
        close(data_sock);
        return -1;
    }

    /* Recibir datos */
    printf("Descargando %s...\n", archivo_remoto);
    while ((n = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, n, fp);
        total += n;
        printf("\rDescargados: %ld bytes", total);
        fflush(stdout);
    }
    printf("\n");

    fclose(fp);
    close(data_sock);

    /* Leer respuesta final */
    leer_respuesta(control_sock, response, sizeof(response));

    printf("Descarga completada: %ld bytes\n", total);
    return 0;
}

/* cmd_stor - Sube un archivo al servidor. */
int cmd_stor(const char* archivo_local, const char* archivo_remoto) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE], buffer[BUFFER_SIZE];
    int data_sock, codigo, n;
    FILE* fp;
    long total = 0;

    /* Verificar que existe el archivo local */
    fp = fopen(archivo_local, "rb");
    if (!fp) {
        fprintf(stderr, "Error: no se puede abrir archivo local: %s\n", archivo_local);
        return -1;
    }

    /* Entrar en modo pasivo */
    data_sock = cmd_pasv();
    if (data_sock < 0) {
        fclose(fp);
        return -1;
    }

    /* Enviar comando STOR */
    snprintf(cmd, sizeof(cmd), "STOR %s", archivo_remoto);
    if (enviar_comando(control_sock, cmd) < 0) {
        close(data_sock);
        fclose(fp);
        return -1;
    }

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    if (codigo != 150 && codigo != 125) {
        fprintf(stderr, "Error: servidor no aceptó transferencia\n");
        close(data_sock);
        fclose(fp);
        return -1;
    }

    /* Enviar datos */
    printf("Subiendo %s...\n", archivo_local);
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send(data_sock, buffer, n, 0) < 0) {
            fprintf(stderr, "Error enviando datos\n");
            break;
        }
        total += n;
        printf("\rSubidos: %ld bytes", total);
        fflush(stdout);
    }
    printf("\n");

    fclose(fp);
    close(data_sock);

    /* Leer respuesta final */
    leer_respuesta(control_sock, response, sizeof(response));

    printf("Subida completada: %ld bytes\n", total);
    return 0;
}

/* Comandos adicionales */

/* cmd_pwd - Muestra el directorio de trabajo actual en el servidor. */
int cmd_pwd() {
    char response[BUFFER_SIZE];
    int codigo;

    if (enviar_comando(control_sock, "PWD") < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));

    if (codigo == 257) {
        printf("Directorio actual mostrado arriba\n");
        return 0;
    }

    return -1;
}

/* cmd_mkd - Crea un directorio en el servidor. */
int cmd_mkd(const char* dirname) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE];
    int codigo;

    snprintf(cmd, sizeof(cmd), "MKD %s", dirname);
    if (enviar_comando(control_sock, cmd) < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));

    if (codigo == 257) {
        printf("Directorio creado exitosamente\n");
        return 0;
    }

    fprintf(stderr, "Error creando directorio\n");
    return -1;
}

/* cmd_dele - Elimina un archivo en el servidor. */
int cmd_dele(const char* filename) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE];
    int codigo;

    snprintf(cmd, sizeof(cmd), "DELE %s", filename);
    if (enviar_comando(control_sock, cmd) < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));

    if (codigo == 250) {
        printf("Archivo eliminado exitosamente\n");
        return 0;
    }

    fprintf(stderr, "Error eliminando archivo\n");
    return -1;
}

/* cmd_cwd - Cambia el directorio de trabajo en el servidor. */
int cmd_cwd(const char* dirname) {
    char cmd[CMD_SIZE], response[BUFFER_SIZE];
    int codigo;

    snprintf(cmd, sizeof(cmd), "CWD %s", dirname);
    if (enviar_comando(control_sock, cmd) < 0)
        return -1;

    codigo = leer_respuesta(control_sock, response, sizeof(response));

    if (codigo == 250) {
        printf("Directorio cambiado exitosamente\n");
        return 0;
    }

    fprintf(stderr, "Error cambiando directorio\n");
    return -1;
}

/* cmd_list - Lista los archivos en el directorio actual del servidor. */
int cmd_list() {
    char response[BUFFER_SIZE], buffer[BUFFER_SIZE];
    int data_sock, codigo, n;

    /* Entrar en modo pasivo */
    data_sock = cmd_pasv();
    if (data_sock < 0)
        return -1;

    /* Enviar comando LIST */
    if (enviar_comando(control_sock, "LIST") < 0) {
        close(data_sock);
        return -1;
    }

    codigo = leer_respuesta(control_sock, response, sizeof(response));
    if (codigo != 150 && codigo != 125) {
        fprintf(stderr, "Error: servidor no inició listado\n");
        close(data_sock);
        return -1;
    }

    /* Recibir y mostrar listado */
    printf("\n=== LISTADO DE ARCHIVOS ===\n");
    while ((n = recv(data_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    printf("===========================\n\n");

    close(data_sock);

    /* Leer respuesta final */
    leer_respuesta(control_sock, response, sizeof(response));

    return 0;
}

/* transferencia_concurrente - Crea un proceso hijo para manejar una transferencia de archivos. */
void transferencia_concurrente(const char* tipo, const char* archivo1,
    const char* archivo2) {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Error en fork()\n");
        return;
    }

    if (pid == 0) {
        /* Proceso hijo - realizar transferencia */
        printf("[PID %d] Iniciando %s de %s\n", getpid(), tipo, archivo1);

        if (strcmp(tipo, "RETR") == 0) {
            cmd_retr(archivo1, archivo2);
        }
        else if (strcmp(tipo, "STOR") == 0) {
            cmd_stor(archivo1, archivo2);
        }

        printf("[PID %d] Transferencia completada\n", getpid());
        exit(0);
    }
    /* Padre continúa */
}

/* menu_interactivo - Muestra el menú principal y maneja la interacción con el usuario. */
void menu_interactivo() {
    char opcion[10], archivo1[256], archivo2[256];
    int continuar = 1;

    while (continuar) {
        printf("\n=== CLIENTE FTP CONCURRENTE ===\n");
        printf("1. Descargar archivo (RETR)\n");
        printf("2. Subir archivo (STOR)\n");
        printf("3. Descargar múltiples archivos (concurrente)\n");
        printf("4. Subir múltiples archivos (concurrente)\n");
        printf("5. Listar archivos (LIST)\n");
        printf("6. Mostrar directorio actual (PWD)\n");
        printf("7. Crear directorio (MKD)\n");
        printf("8. Cambiar directorio (CWD)\n");
        printf("9. Eliminar archivo (DELE)\n");
        printf("0. Salir (QUIT)\n");
        printf("Opción: ");

        if (fgets(opcion, sizeof(opcion), stdin) == NULL)
            break;

        switch (opcion[0]) {
        case '1':
            printf("Archivo remoto: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo local: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            cmd_retr(archivo1, archivo2);
            break;

        case '2':
            printf("Archivo local: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo remoto: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            cmd_stor(archivo1, archivo2);
            break;

        case '3':
            printf("Descarga concurrente de múltiples archivos\n");
            printf("Archivo remoto 1: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo local 1: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            transferencia_concurrente("RETR", archivo1, archivo2);

            printf("Archivo remoto 2: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo local 2: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            transferencia_concurrente("RETR", archivo1, archivo2);

            /* Esperar a que terminen los hijos */
            printf("Esperando transferencias...\n");
            while (wait(NULL) > 0);
            printf("Todas las descargas completadas\n");
            break;

        case '4':
            printf("Subida concurrente de múltiples archivos\n");
            printf("Archivo local 1: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo remoto 1: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            transferencia_concurrente("STOR", archivo1, archivo2);

            printf("Archivo local 2: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            printf("Archivo remoto 2: ");
            fgets(archivo2, sizeof(archivo2), stdin);
            archivo2[strcspn(archivo2, "\n")] = 0;

            transferencia_concurrente("STOR", archivo1, archivo2);

            /* Esperar a que terminen los hijos */
            printf("Esperando transferencias...\n");
            while (wait(NULL) > 0);
            printf("Todas las subidas completadas\n");
            break;

        case '5':
            cmd_list();
            break;

        case '6':
            cmd_pwd();
            break;

        case '7':
            printf("Nombre del directorio: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            cmd_mkd(archivo1);
            break;

        case '8':
            printf("Directorio destino: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            cmd_cwd(archivo1);
            break;

        case '9':
            printf("Archivo a eliminar: ");
            fgets(archivo1, sizeof(archivo1), stdin);
            archivo1[strcspn(archivo1, "\n")] = 0;

            cmd_dele(archivo1);
            break;

        case '0':
            enviar_comando(control_sock, "QUIT");
            continuar = 0;
            break;

        default:
            printf("Opción inválida\n");
        }
    }
}

/* main - Función principal del cliente FTP. */
int main(int argc, char* argv[]) {
    char response[BUFFER_SIZE];
    char username[128], password[128];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <servidor>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s ftp.example.com\n", argv[0]);
        exit(1);
    }

    strcpy(server_host, argv[1]);

    printf("=== CLIENTE FTP CONCURRENTE ===\n");
    printf("Conectando a %s...\n", server_host);

    /* Conectar al servidor FTP (puerto 21) */
    control_sock = connectTCP(server_host, "21");
    printf("Conectado!\n\n");

    /* Leer mensaje de bienvenida */
    leer_respuesta(control_sock, response, sizeof(response));

    /* Autenticación */
    printf("Usuario: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    if (cmd_user(username) < 0) {
        fprintf(stderr, "Error en autenticación\n");
        close(control_sock);
        exit(1);
    }

    printf("Contraseña: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    if (cmd_pass(password) < 0) {
        fprintf(stderr, "Error: contraseña incorrecta\n");
        close(control_sock);
        exit(1);
    }

    printf("\n¡Autenticación exitosa!\n");

    /* Menú interactivo */
    menu_interactivo();

    /* Cerrar conexión */
    close(control_sock);
    printf("Conexión cerrada. ¡Adiós!\n");

    return 0;
}
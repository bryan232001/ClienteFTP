/* connectTCP.c - Función para conectar usando TCP */

extern int connectsock(const char *host, const char *service, 
                       const char *transport);

/* connectTCP - conectar a un servicio especificado en un host especificado*/
int connectTCP(const char *host, const char *service) {
    return connectsock(host, service, "tcp");
}
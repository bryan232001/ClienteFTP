/*  Función para crear y conectar sockets */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

extern int errexit(const char*, ...);

/*connectsock - asignar y conectar un socket usando TCP o UDP*/

int connectsock(const char* host, const char* service, const char* transport) {
    struct hostent* phe;
    struct servent* pse;
    struct protoent* ppe;
    struct sockaddr_in sin;
    int s, type;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    /* Mapear nombre del servicio a número de puerto */
    pse = getservbyname(service, transport);
    if (pse)
        sin.sin_port = pse->s_port;
    else if ((sin.sin_port = htons((unsigned short)atoi(service))) == 0)
        errexit("no se puede obtener el servicio \"%s\"\n", service);

    /* Mapear nombre del host a dirección IP */
    phe = gethostbyname(host);
    if (phe)
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
        errexit("no se puede obtener el host \"%s\"\n", host);

    /* Mapear nombre del protocolo a número de protocolo */
    if ((ppe = getprotobyname(transport)) == 0)
        errexit("no se puede obtener el protocolo \"%s\"\n", transport);

    /* Usar número de protocolo para elegir el tipo de socket */
    if (strcmp(transport, "udp") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    /* Asignar un socket */
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
        errexit("no se puede crear el socket: %s\n", strerror(errno));

    /* Conectar el socket */
    if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errexit("no se puede conectar al %s.%s: %s\n", host, service,
            strerror(errno));
    return s;
}
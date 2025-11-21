/* errexit.c - Función para imprimir error y terminar */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*errexit - imprimir un mensaje de error y terminar*/
int errexit(const char *format, ...) {
    va_list args;
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}
# Cliente FTP Concurrente

Un cliente FTP desarrollado en C que permite transferir archivos desde y hacia servidores FTP, con la capacidad de manejar múltiples transferencias simultáneas.

## ¿Qué hace?

Este programa se conecta a servidores FTP y permite descargar y subir archivos. Lo interesante es que puede hacer varias transferencias al mismo tiempo sin tener que esperar a que termine una para empezar otra. Esto lo logra usando procesos separados (fork) para cada transferencia.

## Compilación

Para compilar el proyecto solo necesitas ejecutar:

```bash
make
```

Esto genera el ejecutable `clienteFTP`.

## Uso básico

Para conectarte a un servidor FTP:

```bash
./clienteFTP <servidor>
```

Por ejemplo:

```bash
./clienteFTP test.rebex.net
```

El programa te pedirá usuario y contraseña. Para el servidor de prueba `test.rebex.net` puedes usar:
- Usuario: demo
- Password: password

## Funcionalidades

Una vez conectado, el programa ofrece las siguientes opciones:

**Operaciones básicas:**
- Descargar archivos individuales
- Subir archivos individuales
- Descargar múltiples archivos simultáneamente (concurrencia)
- Subir múltiples archivos simultáneamente (concurrencia)

**Operaciones adicionales:**
- Listar archivos del directorio actual
- Mostrar directorio de trabajo actual
- Crear directorios
- Cambiar de directorio
- Eliminar archivos

## Cómo funciona la concurrencia

Cuando se selecciona la opción de transferir múltiples archivos, el programa crea un proceso hijo independiente para cada transferencia usando fork(). Esto permite que:

- El proceso padre mantenga activa la conexión de control y el menú
- Cada proceso hijo maneje su propia transferencia de forma independiente
- Las transferencias ocurran en paralelo, aprovechando mejor los recursos

Puedes verificar que está funcionando porque cada transferencia muestra un PID (Process ID) diferente en pantalla.

## Comandos FTP implementados

**Comandos básicos (requeridos):**
- USER: Autenticación de usuario
- PASS: Envío de contraseña
- RETR: Descarga de archivos
- STOR: Subida de archivos
- PASV: Modo pasivo para transferencia de datos

**Comandos adicionales **
- LIST: Listar archivos y directorios
- PWD: Mostrar directorio actual
- MKD: Crear directorio
- CWD: Cambiar directorio
- DELE: Eliminar archivo

## Arquitectura del proyecto

El cliente está dividido en varios archivos:

- `YungaB-clienteFTP.c`: Programa principal con la lógica del cliente
- `connectsock.c`: Función genérica para crear sockets
- `connectTCP.c`: Función específica para conexiones TCP
- `errexit.c`: Manejo de errores
- `Makefile`: Automatización de la compilación

## Detalles técnicos

El programa utiliza el protocolo FTP según se define en el RFC 959. Mantiene dos tipos de conexiones:

- **Conexión de control** (puerto 21): Para enviar comandos
- **Conexión de datos** (modo PASV): Para transferir archivos

Las transferencias se realizan en bloques de 4096 bytes, lo que permite manejar archivos de cualquier tamaño sin cargar todo el contenido en memoria.

## Servidores de prueba

Para probar el cliente puedes usar:

**test.rebex.net**
- Usuario: demo
- Password: password
- Permite descargas

**ftp.dlptest.com**
- Usuario: dlpuser
- Password: rNrKYTX9g7z3RgJRmxWuGHbeu
- Permite descargas y subidas

## Notas importantes

- El programa está diseñado para funcionar en sistemas Linux/Unix o WSL (Windows Subsystem for Linux)
- Requiere GCC y las herramientas de compilación estándar
- La concurrencia se implementa mediante procesos (fork), no hilos
- El modo pasivo (PASV) es el único modo de transferencia implementado, ya que es el más compatible con firewalls y NAT

## Autor

Bryan Yunga  
Proyecto de Computación Distribuida 
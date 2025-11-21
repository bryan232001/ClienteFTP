# Makefile para Cliente FTP Concurrente

CC = gcc
CFLAGS = -Wall -g
TARGET = clienteFTP
OBJS = YungaB-clienteFTP.o connectsock.o connectTCP.o errexit.o

# Regla principal
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Compilación exitosa!"
	@echo "Para ejecutar: ./$(TARGET) <servidor>"

# Compilar archivos objeto
YungaB-clienteFTP.o: YungaB-clienteFTP.c
	$(CC) $(CFLAGS) -c YungaB-clienteFTP.c

connectsock.o: connectsock.c
	$(CC) $(CFLAGS) -c connectsock.c

connectTCP.o: connectTCP.c
	$(CC) $(CFLAGS) -c connectTCP.c

errexit.o: errexit.c
	$(CC) $(CFLAGS) -c errexit.c

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Archivos limpiados"

# Prueba rápida 
test: $(TARGET)
	@echo "Para probar, ejecuta:"
	@echo "./$(TARGET) test.rebex.net"
	@echo "Usuario: demo"
	@echo "Password: password"

.PHONY: all clean test

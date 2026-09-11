#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SIZE 16384
#define CS registros[26]
#define DS registros[27]
// Memoria y Registros
uint8_t memoria[SIZE];
int32_t registros[32];

// Tabla de segmentos: 8 entradas de 32 bits (base 16 bits, tamaño 16 bits)
typedef struct {
    uint16_t base;
    uint16_t tamano;
} Segmento;

Segmento tabla_segmentos[8];

int iniciar_programa(const char *ruta_archivo) {
    FILE *archivo = fopen(ruta_archivo, "rb");
    if (!archivo) {
        printf("Error al abrir el archivo .vmx");
        return 0;
    }

    // 1. Leer y validar cabecera
    char cabecera[6] = {0};
    if (fread(cabecera, sizeof(char), 5, archivo) != 5 || strcmp(cabecera, "VMX26") != 0) {
        printf("Error: Archivo no valido o formato irreconocible.\n");
        fclose(archivo);
        return 0;
    }

    uint8_t version;
    fread(&version, sizeof(uint8_t), 1, archivo);
    if (version != 1) {
        printf("Error: Version no soportada (%d).\n", version);
        fclose(archivo);
        return 0;
    }

    // 2. Tamaño del código (2 bytes)
    uint8_t tam_bytes[2];
    fread(tam_bytes, sizeof(uint8_t), 2, archivo);
    uint16_t tam_codigo = (tam_bytes[0] << 8) | tam_bytes[1];

    // 3. Cargar el código directamente al inicio de la memoria RAM
    size_t leidos = fread(memoria, sizeof(uint8_t), tam_codigo, archivo);
    fclose(archivo);

    if (leidos != tam_codigo) {
        printf("Error: No se pudo leer todo el segmento de código.\n");
        return 0;
    }

    tabla_segmentos[0].base = 0;
    tabla_segmentos[0].tamano = tam_codigo;

    // Segmento 1: Datos (el resto de la RAM)
    tabla_segmentos[1].base = tam_codigo;
    tabla_segmentos[1].tamano = SIZE - tam_codigo;

    // Segmentos 2 al 7: no utilizados por ahora (valor 0xFFFF)
    for (int i = 2; i < 8; i++) {
        tabla_segmentos[i].base = 0xFFFF;
        tabla_segmentos[i].tamano = 0xFFFF;
    }

    // Inicializar registros base 
    CS = 0x00000000; // CS
    DS = 0x00010000; // DS
    registros[0]  = registros[26]; // IP

    return 1;
}
int main(){
    iniciar_programa("arch.vmx");
    /*para probar lecturas
    printf("Tamano del codigo: %u bytes\n", tabla_segmentos[0].tamano);
    printf("Bytes cargados en memoria:\n");
    for (uint16_t i = 0; i < tabla_segmentos[0].tamano; i++) {
        printf("[%04X]: %02X (%d)\n", i, memoria[i], memoria[i]);
    }
    */
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Tipos.h"
#include "Memoria.c"
#include "Operaciones1.c"
#include "Operaciones2.c"
#include "Disassembler.c"

int main(int argc, char **argv){
    char* nombre_archivo;
    int flag_on = 0;
    char err_msg[40];
    int err;

    if(argc<2 || argc>3) // si tiene 1 o 2 parametros (argv[0] siempre tiene la direccion del exe)
    {
        printf("INPUT ERROR: cantidad de argumentos invalida, formato de ejecucion:  vmx filename.vmx [-d]\n");
        return 1;
    }
    else if(!str_termina_con(argv[1], ".vmx"))
    {
        printf("INPUT ERROR: parametro 1 (nombre de archivo .vmx) no termina con .vmx\n");
        return 1;
    }
    else if(argc==3 && strcmp(argv[2], "-d"))
    {
        printf("INPUT ERROR: parametro 2 no es -d (unica opcion para parametro 2)\n");
        return 1;
    }
    else if(argc==3)
    {
        flag_on = 1;
    }

    nombre_archivo = argv[1]; // asigno a nombre_archivo el puntero al argumento 1 (path al .asm)

    if(err = iniciar_programa(nombre_archivo))
        printf("ERROR LECTURA: codigo de error: %d\n", err);// deberiamos tener alguna forma de mostrar en texto lo que paso




    while (!err) { // si IP sale del segmento, sucede un error de segmento y retorna error -1
        err=lectura_programa();
    }
    errcode_a_str(err, err_msg);
    printf("\n Deteniendo ejecucion. %s\n\n", err_msg);


    if(flag_on)
    {
        printf("\n Disassembler:\n\n");
        if(err=disassembler())
            printf("disassembler salio con error! (%d)\n", err);
    }
    /*
    printf("\n\n Registros:\n");
    for(int i=0; i<32; i++)
        printf(" [%02X]: %08X %d\n", i, registros[i], registros[i]);

    printf("\n Code Segment: \n");
    int dir_cs, tam_cs;
    puntero_logico_a_direccion_fisica(CS, &dir_cs);
    puntero_logico_a_direccion_fisica(DS, &tam_cs);
    for(int i=dir_cs; i<dir_cs+tam_cs; i++)
        printf(" [%02X]: %02X %d\n", i, memoria[i], memoria[i]);

    printf("\n Data Segment: \n");
    int dir_ds, c_bytes = 16;
    puntero_logico_a_direccion_fisica(DS, &dir_ds);
    for(int i=dir_ds; i<dir_ds+c_bytes; i++)
        printf(" [%02X]: %02X %d\n", i, memoria[i], memoria[i]);
    printf(" ...\n");
    */
    return 0;
}

int str_termina_con(char* str, char* sufijo)
{
    if(!str || !sufijo)
    {
        printf("ERROR: str_termina_con(str, sufijo) recibio un parametro nulo (str: %p, sufijo: %p)\n", str, sufijo);
        return 0;
    }
    int len_str = strlen(str);
    int len_sufijo = strlen(sufijo);
    if(len_str<len_sufijo)
        return 0;
    return strncmp(str + len_str - len_sufijo, sufijo, len_sufijo) == 0;
}

int iniciar_programa(const char *ruta_archivo){

    FILE *archivo = fopen(ruta_archivo, "rb");
    if (!archivo) {
        printf("Error al abrir el archivo %s", ruta_archivo);
        return 1;
    }

    // 1. Leer y validar cabecera
    char cabecera[6] = {0};
    if (fread(cabecera, sizeof(char), 5, archivo) != 5 || strcmp(cabecera, "VMX26") != 0) {
        printf("Error: Archivo no valido o formato irreconocible.\n");
        fclose(archivo);
        return 2;
    }

    uint8_t version;
    fread(&version, sizeof(uint8_t), 1, archivo);
    if (version != 1) {
        printf("Error: Version no soportada (%d).\n", version);
        fclose(archivo);
        return 3;
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
        return 4;
    }
    //Segmento 0: Codigo
    tabla_segmentos[0] = ((uint32_t)0 << 16) | tam_codigo;

    // Segmento 1: Datos (el resto de la RAM)
    tabla_segmentos[1] = (uint32_t)tam_codigo<<16;
    tabla_segmentos[1] = tabla_segmentos[1] | (uint32_t)(SIZE - tam_codigo);

    // Segmentos 2 al 7: no utilizados por a hora (valor 0xFFFFFFFF)
    for (int i = 2; i < 8; i++) {
        tabla_segmentos[i] = 0xFFFFFFFF;
    }

    // Inicializar registros base
    CS = 0x00000000; // CS
    DS = 0x00010000; // DS
    IP = CS; // IP

    return 0;
}

/**
 * retorna en out un mensaje explicando el codigo de error err
 * 
 * @param err codigo de error
 * @param out puntero a cadena de al menos 40 chars
 */
void errcode_a_str(int err, char* out)
{
    switch(err)
    {
        case 1:
            strcpy(out, "Fin de programa");
            break;
        case -1:
            strcpy(out, "Fallo de segmento");
            break;
        case -2:
            strcpy(out, "Instruccion invalida");
            break;
        case -3:
            strcpy(out, "Division por cero");
            break;
        default:
            sprintf(out, "Error de programacion, errcode: %d", err);
    }
}

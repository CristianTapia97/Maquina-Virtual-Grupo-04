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
    char* nombre_archivo_vmx = NULL;    //       [filename.vmx]
    char* nombre_archivo_vmi = NULL;    //       [filename.vmi]
    int32_t memsize=-1;                 //       [m=M]
    int hay_disassembler = 0;           //       [-d]
    int hay_params = 0;                 //       [-p]
    int paramsc = 0;                    //       cantidad de parametros -p
    char** paramsv = NULL;              //       [param1, param2, param3, ...]

    char err_msg[40];
    int err=0;
    
    // inicio lectura de argumentos de ejecucion
    for(int i=1; i<argc; i++)
    {
        if(str_termina_con(argv[i], ".vmx"))
        {
            nombre_archivo_vmx = malloc(strlen(argv[i])+1);
            strcpy(nombre_archivo_vmx, argv[i]);
        }
        else if(str_termina_con(argv[i], ".vmi"))
        {
            nombre_archivo_vmi = malloc(strlen(argv[i])+1);
            strcpy(nombre_archivo_vmi, argv[i]);
        }
        else if(str_arranca_con(argv[i], "m="))
        {
            if(sscanf(argv[i], "m=%d", &memsize)!=1)
            {
                printf("INPUT ERROR: argumento m='numero' no tubo un numero: %s\n", argv[i]);
                err = 1;
                break;
            }
            if(memsize<=0)
            {
                printf("INPUT ERROR: argumento m=%d recibio un numero negativo\n", memsize);
                err = 1;
                break;
            }
        }
        else if(strcmp(argv[i], "-d")==0)
        {
            hay_disassembler=1;
        }
        else if(strcmp(argv[i], "-p")==0)
        {
            hay_params = 1;
            paramsc = argc-(i+1);
            break; // a partir de -p, todos los siguientes args son parametros para el assembler
        }
    }

    if(nombre_archivo_vmx==NULL && nombre_archivo_vmi==NULL)
    {
        err=1;
        printf("INPUT ERROR: no se ingreso un archivo .vmx ni un archivo .vmi\n", memsize);
    }
    else if(hay_params && nombre_archivo_vmx==NULL)
    {
        err=1;
        printf("INPUT ERROR: no se permite el uso de parametros -p cargando el programa desde una imagen\n", memsize); // esto porque los parametros van en memoria, pero los archivos .vmi contienen la memoria entera, asi que los parametros no se usarian
    }
    
    if(err)
        return -1;

    if(hay_params) // armo paramsv con todos los parametros ingresados
    {
        int index_p1 = argc-paramsc;
        char* param;

        paramsv = malloc(sizeof(char*)*(paramsc+1)); // +1 para contemplar el archivo 

        paramsv[0] = malloc(sizeof(char)*(strlen(nombre_archivo_vmx)+1));
        strcpy(paramsv[0], nombre_archivo_vmx);

        for(int i=0; i<paramsc; i++)
        {
            param = argv[index_p1+i];
            paramsv[1+i] = malloc(sizeof(char)*strlen(param));
            strcpy(paramsv[1+i], param);
        }
    }
    // fin lectura de argumentos de ejecucion

    if(nombre_archivo_vmx)
        if(err = iniciar_programa(nombre_archivo_vmx, memsize, paramsc, paramsv))
            printf("ERROR ARRANQUE VMX: codigo de error: %d\n", err);
    else
        if(err = cargar_programa_imagen(nombre_archivo_vmi))
            printf("ERROR ARRANQUE VMI: codigo de error: %d\n", err);




    while (!err) { // si IP sale del segmento, sucede un error de segmento y retorna error -1
        err=lectura_programa();
    }
    errcode_a_str(err, err_msg);
    printf("\n Deteniendo ejecucion. %s\n\n", err_msg);


    if(hay_disassembler)
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

int str_arranca_con(char* str, char* prefijo)
{
    if(!str || !prefijo)
    {
        printf("ERROR: str_arranca_con(str, prefijo) recibio un parametro nulo (str: %p, prefijo: %p)\n", str, prefijo);
        return 0;
    }
    int len_str = strlen(str);
    int len_prefijo = strlen(prefijo);
    if(len_str<len_prefijo)
        return 0;
    return strncmp(str, prefijo, len_prefijo) == 0;
}

// falta adaptar esto a el nuevo formato de archivo vmx
int iniciar_programa(const char *ruta_archivo, int32_t memsize, int paramsc, char** paramsv){
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

// primero habria que hacer que el programa pueda ejecutar el nuevo formato de vmx y generar imagenes vmi, para hacer las cosas progresivamente y no volvernos locos
int cargar_programa_imagen(const char *nombre_archivo_vmi)
{

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

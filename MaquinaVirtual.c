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
    int32_t memsize=16;                 //       [m=M]
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
                printf("INPUT ERROR: argumento m='numero' no tuvo un numero: %s\n", argv[i]);
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
            paramsv[1+i] = malloc(sizeof(char)*strlen(param) + 1);
            strcpy(paramsv[1+i], param);
        }
    }
    // fin lectura de argumentos de ejecucion
    memsize *= 1024;
    memoria = (uint8_t*) calloc(memsize, sizeof(uint8_t));
    if (memoria == NULL) {
        printf("ERROR: No se pudo asignar memoria principal (%d bytes).\n", memsize);
        return ERR_MI;
    }

    if (nombre_archivo_vmx != NULL) {
        err = iniciar_programa(nombre_archivo_vmx, memsize, paramsc, paramsv);
        if (err) {
            printf("ERROR ARRANQUE VMX: codigo de error: %d\n", err);
            if (memoria != NULL) { 
                free(memoria); 
                memoria = NULL; 
            }
            return err;
        }
    } else if (nombre_archivo_vmi != NULL) {
        err = cargar_programa_imagen(nombre_archivo_vmi);
        int tienevmi=1;
        if (err) {
            printf("ERROR ARRANQUE VMI: codigo de error: %d\n", err);
            if (memoria != NULL) { 
                free(memoria); 
                memoria = NULL; 
            }
            return err;
        }
    }

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

    if(PS!=0xFFFFFFFF)
    {
        printf("\n Param Segment: ");
        int dir_ps = tabla_segmentos[PS>>16]&0xFFFF0000;
        dir_ps >>= 16;
        int tam_ps = tabla_segmentos[PS>>16]&0x0000FFFF;

        for(int i=dir_ps; i<dir_ps+tam_ps; i++)
        {
            printf("\n [%02X]: %02X", i, memoria[i]);
            if(memoria[i]>=32)
                printf(" %c", memoria[i]);
        }
    }

    printf("\n Stack Segment: ");
    int dir_ss = tabla_segmentos[SS>>16]&0xFFFF0000;
    dir_ss >>= 16;
    int tam_ss = tabla_segmentos[SS>>16]&0x0000FFFF;

    /*for(int i=dir_ss; i<dir_ss+tam_ss; i++)
    {
        printf("\n [%02X]: %02X", i, memoria[i]);
        if(memoria[i]>=32)
            printf(" %c", memoria[i]);
    }*/
    

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
    if(memoria != NULL) {
        free(memoria);
        memoria = NULL;
    }
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
    if (version != 1 && version !=2) {
        printf("Error: Version no soportada (%d).\n", version);
        fclose(archivo);
        return 3;
    }

    // 2. Tamaño del código (2 bytes)
    uint8_t tam_bytes[2];
    uint16_t tam_cs = 0, tam_ds = 0, tam_es = 0, tam_ss = 0, tam_ks = 0, entry_point = 0;
    if (version == 1) {
        // En V1 solo hay 2 bytes de tam_codigo (CS)
        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_cs = (tam_bytes[0] << 8) | tam_bytes[1];
        tam_ds = memsize - tam_cs;
    }
    else if (version == 2) {
        // En V2 leemos los 6 campos de 2 bytes:
        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_cs = (tam_bytes[0] << 8) | tam_bytes[1];

        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_ds = (tam_bytes[0] << 8) | tam_bytes[1];

        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_es = (tam_bytes[0] << 8) | tam_bytes[1];

        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_ss = (tam_bytes[0] << 8) | tam_bytes[1];

        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        tam_ks = (tam_bytes[0] << 8) | tam_bytes[1];

        fread(tam_bytes, sizeof(uint8_t), 2, archivo);
        entry_point = (tam_bytes[0] << 8) | tam_bytes[1];
    }
    uint16_t tam_ps = 0;
    int cant_args = (paramsv != NULL && paramsc > 0) ? (paramsc + 1) : 0;
    if (cant_args > 0) {
        for (int i = 0; i < cant_args; i++) {
            tam_ps += (uint16_t)(strlen(paramsv[i]) + 1);
        }
        tam_ps += (uint16_t)(cant_args * 4); // vector de punteros argv
    }

    uint32_t total_necesario = (uint32_t)tam_ps + tam_ks + tam_cs + tam_ds + tam_es + tam_ss;
    if (total_necesario > (uint32_t)memsize) {
        printf("ERROR: Memoria insuficiente. Requerido: %u bytes, Disponible: %d bytes.\n", total_necesario, memsize);
        fclose(archivo);
        return ERR_MI;
    }

    uint32_t dir_ps = 0;
    uint32_t dir_ks = dir_ps + tam_ps;
    uint32_t dir_cs = dir_ks + tam_ks;
    uint32_t dir_ds = dir_cs + tam_cs;
    uint32_t dir_es = dir_ds + tam_ds;
    uint32_t dir_ss = dir_es + tam_es;
    if (tam_cs > 0) {
        if (fread(&memoria[dir_cs], sizeof(uint8_t), tam_cs, archivo) != tam_cs) {
            printf("Error al leer el segmento CS del archivo.\n");
            fclose(archivo);
            return 4;
        }
    }
    if (tam_ks > 0) {
        if (fread(&memoria[dir_ks], sizeof(uint8_t), tam_ks, archivo) != tam_ks) {
            printf("Error al leer el segmento KS del archivo.\n");
            fclose(archivo);
            return 4;
        }
    }
    fclose(archivo);

    // Armar tabla_segmentos con los segmentos que tengan tamaño > 0
    for (int i = 0; i < 8; i++)
        tabla_segmentos[i] = 0xFFFFFFFF;

    PS = -1; KS = -1; CS = -1; DS = -1; ES = -1; SS = -1;

    int i = 0;

    if (tam_ps > 0) {
        tabla_segmentos[i] = (dir_ps << 16) | tam_ps;
        PS = (i << 16);
        i++;
    }
    if (tam_ks > 0) {
        tabla_segmentos[i] = (dir_ks << 16) | tam_ks;
        KS = (i << 16);
        i++;
    }
    if (tam_cs > 0) {
        tabla_segmentos[i] = (dir_cs << 16) | tam_cs;
        CS = (i << 16);
        i++;
    }
    if (tam_ds > 0) {
        tabla_segmentos[i] = (dir_ds << 16) | tam_ds;
        DS = (i << 16);
        i++;
    }
    if (tam_es > 0) {
        tabla_segmentos[i] = (dir_es << 16) | tam_es;
        ES = (i << 16);
        i++;
    }
    if (tam_ss > 0) {
        tabla_segmentos[i] = (dir_ss << 16) | tam_ss;
        SS = (i << 16);
        i++;
    }

    IP = CS | (entry_point & 0xFFFF);

    //inicializacion de el param segment (era mas lindo arriba pero nesecita la tabla de segmentos inicializada)
    uint32_t lp_argv = -1; // logical pointer al argv en el param segment
    if(tam_ps > 0) // muevo la inicialisacion al final porque nesecito a la pila inicializada
    {
        int offset_ps = 0;
        uint32_t offsets_params[cant_args];
        for(int i=0; i<cant_args; i++) {
            offsets_params[i] = offset_ps; // como el param segment es el primer segmento, los punteros logicos son iguales a las posiciones fisicas
            for(int j=0; j<strlen(paramsv[i]); j++) {
                memoria[offset_ps] = paramsv[i][j];
                offset_ps++;
            }
            memoria[offset_ps] = '\0';
            offset_ps++;
        }
        lp_argv = offset_ps; 
        for(int i=0; i<cant_args; i++) {
            printf("offsets_params[%d]: %d\n",i, offsets_params[i]);
            escribir_memoria(offset_ps, 4, offsets_params[i]);
            //para no cargar los reistros lar, mar y mbr con basura
            LAR = 0;
            MAR = 0;
            MBR = 0;
            offset_ps+=4;
        }
    }
    printf("lp_argv: %08X\n", lp_argv);

    //inicializacion de la pilas, por mas que no tenga definidos parametros, se pushean igual la direccion de retorno de la subrutina principal (0 para argc y -1 para argv)
    SP = SS + tam_ss;
    SP-=4;
    escribir_memoria(SP, 4, lp_argv);
    SP-=4;
    escribir_memoria(SP, 4, cant_args);
    SP-=4;
    escribir_memoria(SP, 4, -1); // RET (-1)
    //para no cargar los reistros lar, mar y mbr con basura
    LAR = 0;
    MAR = 0;
    MBR = 0;

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

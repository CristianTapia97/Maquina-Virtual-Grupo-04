#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SIZE 16384
#define IP registros[0]
#define OPC registros[1]
#define OP1 registros[2]
#define OP2 registros[3]
#define LAR registros[4]
#define MAR registros[5]
#define MBR registros[6]
#define CS registros[26]
#define DS registros[27]
// Memoria y Registros
uint8_t memoria[SIZE];
int32_t registros[32] = {0};
int32_t tabla_segmentos[8];
// Tabla de segmentos: 8 entradas de 32 bits 

int puntero_logico_a_direccion_fisica(uint32_t puntero_l);
uint32_t leer_memoria(int dir_fisica, uint8_t c_bytes);
int lectura_programa();
int iniciar_programa(const char *ruta_archivo);

int main(){
    iniciar_programa("ej7.vmx");

    /*para probar lecturas*/
    uint16_t tam_codigo = tabla_segmentos[0] & 0xFFFF;

    // Recorre instrucción por instrucción hasta que IP alcance el fin del código o dé error
    while ((IP != (int32_t)0xFFFFFFFF) && ((IP & 0xFFFF) < tam_codigo)) {
        printf("\n[Fetch en IP = %08X]\n", IP);
        if (!lectura_programa()) {
            printf("Deteniendo lectura por error o STOP.\n");
            break;
        }
    }
    return 0;
}
int iniciar_programa(const char *ruta_archivo){
    
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
    //Segmento 0: Codigo
    tabla_segmentos[0] = ((uint32_t)0 << 16) | tam_codigo;

    // Segmento 1: Datos (el resto de la RAM)
    tabla_segmentos[1] = (uint32_t)tam_codigo<<16;
    tabla_segmentos[1] = tabla_segmentos[1] | (uint32_t)(SIZE - tam_codigo);

    // Segmentos 2 al 7: no utilizados por ahora (valor 0xFFFFFFFF)
    for (int i = 2; i < 8; i++) {
        tabla_segmentos[i] = 0xFFFFFFFF;
    }

    // Inicializar registros base 
    CS = 0x00000000; // CS
    DS = 0x00010000; // DS
    registros[0]  = CS; // IP

    return 1;
}

// los return -1 significan que hubo un error, los prints son temporales para el debug
int puntero_logico_a_direccion_fisica(uint32_t puntero_l)
{
    uint16_t cod_segmento = puntero_l >> 16;
    uint16_t offset = puntero_l;                // la mascara 0x0000FFFF esta implisita al pasar de 32 a 16 bits

    if(cod_segmento<0 || cod_segmento>7)
    {
        printf("[ERROR:Fallo de segmento] puntero_logico_a_direccion_fisica(puntero_l): puntero logico %08X con codigo de segmento invalido\n", puntero_l);
        return -1;
    }
    uint32_t segmento = tabla_segmentos[cod_segmento];
    uint16_t pos_inicio_seg = segmento >> 16;
    uint16_t tam_segmento = segmento;           // la mascara 0x0000FFFF esta implisita al pasar de 32 a 16 bits

    int pos_fin_seg = pos_inicio_seg + tam_segmento; // la posicion del primer byte fuera del segmento
    int pos_puntero = pos_inicio_seg + offset;

    if(pos_puntero>=pos_fin_seg || pos_puntero<pos_inicio_seg)
    {
        printf("[ERROR:Fallo de segmento] puntero_logico_a_direccion_fisica(puntero_l): puntero logico %08X apunta fuera de su segmento \n", puntero_l);
        return -1;
    }


    //printf("puntero_l:     %08X\n", puntero_l);
    //printf("cod_segmento:  %04X\n", cod_segmento);
    //printf("offset:        %04X\n\n", offset);
    //printf("segmento:      %08X\n", segmento);

    return pos_puntero;
}


// pueden leerse de 0 a 4 bytes!
// lee los bytes desde la direccion fisica ingresada y los junta en un solo valor, el output es siempre de 4 bytes pero pueden leerse de 0 a 4 bytes
uint32_t leer_memoria(int dir_fisica, uint8_t c_bytes)
{
    if(0>c_bytes || c_bytes>4 || 0>dir_fisica || dir_fisica>=SIZE || dir_fisica+c_bytes-1>=SIZE)
    {
        // este error no deberia suceder nunca, si sucede es por algo mal hecho nuestro, a diferencia de Instruccion invalida Division por cero o Fallo de segmento que son errores del usuario
        printf("ERROR: leer_memoria(dir_fisica, c_bytes) recibio una cantidad invalida de bytes a leer(c_bytes: %d)(0<=c_bytes<=4) o una posicion de la memoria fuera de rango(dir_fisica: %d)(0<=dir_fisica<%d) o dir_final=dir_fisica+c_bytes-1 se escapa de la memoria (dir_final: %d)(0<=dir_final<%d) \n", c_bytes, dir_fisica, SIZE, dir_fisica+c_bytes-1, SIZE);
        return 0xFFFFFFFF;
    }

    uint32_t out = 0;
    for(int i=0; i<c_bytes; i++)
    {
        out <<= 8;
        out += memoria[dir_fisica+i];
    }
    return out;
}
int lectura_programa(){
    uint8_t tipo_p1;
    uint8_t tipo_p2;
    uint32_t data_p1 = 0;
    uint32_t data_p2 = 0;
    uint32_t tam_instruccion = 1;
    int dir_ip = puntero_logico_a_direccion_fisica(IP);
    //if (dir_ip == -1) => Fallo de segmento
    uint8_t operacion = leer_memoria(dir_ip, 1);
    OPC  = operacion & 0b00011111;
    if (OPC == 0x0F) { 
        // 0 operandos: STOP
        tipo_p1 = 0;
        tipo_p2 = 0;
    } 
    else if (OPC <= 0x0A) { 
        // 1 operando (0x00 a 0x0A)
        tipo_p1 = (operacion >> 6) & 0b00000011; // Bits 7 y 6
        tipo_p2 = 0;

        data_p1 = leer_memoria(dir_ip + 1, tipo_p1);
        tam_instruccion += tipo_p1;
    } 
    else if (OPC >= 0x10 && OPC <= 0x1F) { 
        // 2 operandos (0x10 a 0x1F)
        tipo_p2 = (operacion >> 6) & 0b00000011;           // Bits 7 y 6: Operando B
        tipo_p1 = ((operacion >> 4) & 0b00000011); // Operando A

        // Orden en memoria: primero B, luego A
        int pos_data_p2 = dir_ip + 1;
        int pos_data_p1 = pos_data_p2 + tipo_p2;

        data_p2 = leer_memoria(pos_data_p2, tipo_p2);
        data_p1 = leer_memoria(pos_data_p1, tipo_p1);

        tam_instruccion += (tipo_p1 + tipo_p2);
    } 
    else {
        printf("[ERROR: Instruccion invalida] Opcode %02X no existe.\n", OPC); //
        IP = (int32_t)0xFFFFFFFF;
        return 0;
    }
    OP1 = tipo_p1<<24;
    OP1 += data_p1;
    OP2 = tipo_p2<<24;
    OP2 += data_p2;
    IP += tam_instruccion;// desplazo IP a la siguiente instruccion

    printf("operacion: %02X\n", operacion); // out de debug para tantear los valores leidos
    printf("tipo o:    %02X\n", OPC);
    printf("tipo p1:   %d  data: %08X\n", tipo_p1, data_p1);
    printf("tipo p2:   %d  data: %08X\n", tipo_p2, data_p2);
    return 1;
}
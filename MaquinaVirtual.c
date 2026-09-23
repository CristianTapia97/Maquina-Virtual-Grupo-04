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
#define EAX registros[10]
#define EBX registros[11]
#define ECX registros[12]
#define EDX registros[13]
#define EEX registros[14]
#define EFX registros[15]
#define AC registros[16]
#define CC registros[17]
#define CS registros[26]
#define DS registros[27]
#define FLAG_N (1U << 31)
#define FLAG_Z (1U << 30)
#define FLAG_C (1U << 29)
#define FLAG_V (1U << 28)

// Memoria y Registros
uint8_t memoria[SIZE];
int32_t registros[32] = {0};
int32_t tabla_segmentos[8];
// Tabla de segmentos: 8 entradas de 32 bits

char formatos_sys[] = { 'd', 'c', 'o', 'X'/*, 'b' binario se implementa a mano*/ };

//para el registro CC, lleva el bit de signo, de cero, de acarreo y desbordamiento
void set_flags(int n, int z, int c, int v);

// predefinicion operaciones
int opc_stop();

int opc_sys(uint32_t);
int opc_not(uint32_t op1);//afecta el registro CC
int opc_jmp(uint32_t op1);
int opc_jp(uint32_t op1);
int opc_jn(uint32_t op1);
int opc_jz(uint32_t op1);
int opc_jc(uint32_t op1);
int opc_jv(uint32_t op1);
int opc_jnp(uint32_t op1);
int opc_jnn(uint32_t op1);
int opc_jnz(uint32_t op1);
int opc_1placeholder(uint32_t op1){ printf("operacion no implementada. OPC: %02X OP1: %08X\n", OPC, op1); return 0;}

int opc_mov(uint32_t, uint32_t); //afecta el registro CC
int opc_add(uint32_t, uint32_t); //afecta el registro CC
int opc_sub(uint32_t, uint32_t); //afecta el registro CC
int opc_mul(uint32_t, uint32_t); //afecta el registro CC
int opc_div(uint32_t, uint32_t); //afecta el registro CC
int opc_and(uint32_t, uint32_t); //afecta el registro CC
int opc_or(uint32_t, uint32_t); //afecta el registro CC
int opc_xor(uint32_t, uint32_t); //afecta el registro CC
int opc_swap(uint32_t, uint32_t); //afecta el registro CC
int opc_shl(uint32_t, uint32_t); //afecta el registro CC
int opc_shr(uint32_t, uint32_t); //afecta el registro CC
int opc_sar(uint32_t, uint32_t); //afecta el registro CC
int opc_rnd(uint32_t, uint32_t); //afecta el registro CC
int opc_ldl(uint32_t, uint32_t);
int opc_ldh(uint32_t, uint32_t);
int opc_cmp(uint32_t, uint32_t); //afecta el registro CC
int opc_2placeholder(uint32_t op1, uint32_t op2){ printf("operacion no implementada. OPC: %02X OP1: %08X OP2: %08X\n", OPC, op1, op2); return 0;}


// Tipos de las funciones de 2 y 1 parametros
typedef int(*operacion_2_params)(uint32_t, uint32_t); // para charlar, las funciones realmente no hace falta que reciban parametros, porque tienen accesso a los registros, asi que pueden acceder a OP1 y OP2 a voluntad
typedef int(*operacion_1_param)(uint32_t);

// Arrays de funciones de 1 y 2 parametros
operacion_2_params operaciones_2_params[] = {
    opc_mov,//MOV
    opc_add,//ADD   chau placeholders
    opc_sub,//SUB
    opc_mul,//MUL
    opc_div,//DIV
    opc_cmp,//CMPs
    opc_and,//AND
    opc_or,//OR
    opc_xor,//XOR
    opc_swap,//SWAP
    opc_shl,//SHL
    opc_shr,//SHR
    opc_sar,//SAR
    opc_ldl,//LDL
    opc_ldh,//LDH
    opc_rnd //RND
};
operacion_1_param operaciones_1_param[] = {
    opc_sys,//SYS
    opc_jmp,//JMP
    opc_jp,//JP
    opc_jn,//JN
    opc_jz,//JZ
    opc_jc,//JC
    opc_jv,//JV
    opc_jnp,//JNP
    opc_jnn,//JNN
    opc_jnz,//JNZ
    opc_not //NOT
};

char mnemonicos[0x20][5] = {
    "SYS",
    "JMP",
    "JP",
    "JN",
    "JZ",
    "JC",
    "JV",
    "JNP",
    "JNN",
    "JNZ",
    "NOT",
    "ER-B",
    "ER-C", 
    "ER-D", // hay 4 op invalidos pero simplifica mucho tenerlo asi
    "ER-E",
    "STOP",
    "MOV",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "CMP",
    "AND",
    "OR",
    "XOR",
    "SWAP",
    "SHL",
    "SHR",
    "SAR",
    "LDL",
    "LDH",
    "RND"
};

char registros_nombres[32][4] = {
    "IP",
    "OPC",
    "OP1",
    "OP2",
    "LAR",
    "MAR",
    "MBR",
    "R-7",
    "R-8",
    "R-9",
    "EAX",
    "EBX",
    "ECX",
    "EDX",
    "EEX",
    "EFX",
    "AC",
    "CC",
    "R18",
    "R19",
    "R20",
    "R21",
    "R22", // los Rxx son los registros reservados que ya nos contaran para que son
    "R23",
    "R24",
    "R25",
    "CS",
    "DS",
    "R28",
    "R29",
    "R30",
    "R31"
};



int puntero_logico_a_direccion_fisica(uint32_t puntero_l, int* dir);
int chunk_memoria_valido(uint32_t puntero_l, uint8_t bytes, int* out_pos);
int leer_memoria(uint32_t puntero_l, uint8_t c_bytes, uint32_t* data, int leeInstruccion);
int escribir_memoria(uint32_t puntero_l, uint8_t c_bytes, uint32_t data);
int lectura_programa();
int iniciar_programa(const char *ruta_archivo);
int str_termina_con(char* str, char* sufijo);
void op_a_str(uint32_t op, char* str);

int main(int argc, char **argv){
    char nombre_archivo[] = "ej8.vmx";
    int flag_on = 0;

    /* input del archivo por consola, funciona bien pero lo dejo comentado para testear mas comodo

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
    nombre_archivo = malloc(strlen(argv[1])+1);
    strcpy(nombre_archivo, argv[1]);
    printf("nombre archivo ingresado: %s\n", nombre_archivo);
    printf("flag: %d\n", flag_on);
    */


    iniciar_programa(nombre_archivo);

    /*para probar lecturas*/
    uint16_t tam_codigo = tabla_segmentos[0] & 0xFFFF;
    int err;
    // Recorre instrucción por instrucción hasta que IP alcance el fin del código o dé error
    while ((IP != (int32_t)0xFFFFFFFF) && ((IP & 0xFFFF) < tam_codigo)) {
        //printf("\n[Fetch en IP = %08X]\n", IP);
        if (err=lectura_programa()) {
            printf("Deteniendo lectura por error o STOP. errcode:%d\n", err);
            break;
        }
    }

    /**/
    printf("\n Registros:\n");
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

    // Segmentos 2 al 7: no utilizados por a hora (valor 0xFFFFFFFF)
    for (int i = 2; i < 8; i++) {
        tabla_segmentos[i] = 0xFFFFFFFF;
    }

    // Inicializar registros base
    CS = 0x00000000; // CS
    DS = 0x00010000; // DS
    IP  = CS; // IP

    return 1;
}

/**
 * revisa si el sector de memoria iniciando desde el puntero_l de longitud bytes es valido, si lo es retorna opcionalmente en out_pos la posicion del primer byte en la memoria
 *
 * @param puntero_l puntero logico especificando el sector y inicio del 'chunk'
 * @param bytes tamaño del 'chunk' iniciando en puntero_l, si es 0 no se hace ninguna verificacion
 * @param out_pos se usa como output opcional de la posicion en memoria del inicio del chunk (opcional porque podes asignale NULL sin problemas)
 *
 * @return booleano, 0 = no valido, 1 = valido
 */
int chunk_memoria_valido(uint32_t puntero_l, uint8_t bytes, int* out_pos)
{
    if(bytes==0)
        return 1;

    int16_t cod_segmento = puntero_l >> 16;
    uint16_t offset_primero = puntero_l;
    uint16_t offset_ultimo = offset_primero + bytes - 1;

    if(cod_segmento<0 || cod_segmento>7)
        //comento el print porque el que recibe el error deberia encargarse de interpretarlo
        //printf("[ERROR:Fallo de segmento] puntero_logico_a_direccion_fisica(puntero_l): puntero logico %08X con codigo de segmento invalido\n", puntero_l);
        return 0;

    uint32_t segmento = tabla_segmentos[cod_segmento];
    uint16_t tam_segmento = segmento;
    uint16_t pos_inicio_seg = segmento >> 16;
    uint16_t pos_fin_seg = pos_inicio_seg + tam_segmento; // la posicion del primer byte fuera del segmento

    uint16_t pos_primero_chunk = pos_inicio_seg + offset_primero;
    uint16_t pos_ultimo_chunk = pos_inicio_seg + offset_ultimo;

    if(pos_primero_chunk<pos_inicio_seg || pos_primero_chunk>=pos_fin_seg || pos_ultimo_chunk>=pos_fin_seg)
        return 0;

    if(out_pos)
        *out_pos = (int)pos_primero_chunk;
    return 1;
}

/**
 * calcula la direccion fisica a la que apunta un puntero logico
 *
 * @param puntero_l puntero logico
 * @param dir parametro de salida con la direccion fisica
 *
 * @return codigo de error 0=OK -1=Fallo de segmento
 */
int puntero_logico_a_direccion_fisica(uint32_t puntero_l, int* dir)
{
    if(!chunk_memoria_valido(puntero_l, 1, dir))
        return -1;
    return 0;
}


/**
 * lee de 0 a 4 bytes desde la posicion apuntada por puntero_l
 *
 * @param puntero_l puntero logico al primer byte a leer
 * @param c_bytes cantidad de bytes a leer, si es 0 data va a valer 0
 * @param data output de la informacion leida
 * @param leeInstruccion booleano para identificar si lo que se lee es instruccion o datos
 *
 * @return se retorna el codigo de error de la operacion 0 = sin error // importante la distincion entre el codigo de error y la informacion, antes ambos eran el return
 */
int leer_memoria(uint32_t puntero_l, uint8_t c_bytes, uint32_t* data, int leeInstruccion)
{
    int dir_fisica;
    if(!data)
    {
        printf("ERROR: leer_memoria() recibio data = NULL!!!\n");
        return -10; // error nuestro
    }
    else if(!chunk_memoria_valido(puntero_l, c_bytes, &dir_fisica))
        return -1; // error de segmento

    *data = 0;
    for(int i=0; i<c_bytes; i++)
    {
        *data <<= 8;
        *data += memoria[dir_fisica+i];
    }
    //solo carga los registros LAR, MAR y MBR cuando se leen datos, no instrucciones
    if (!leeInstruccion) {
        LAR = puntero_l; //direccion logica
        MAR = ((uint32_t)c_bytes << 16) | ((uint32_t)dir_fisica & 0xFFFF); //cantidad de bytes en los dos bytes mas significativos
        //y direccion fisica en los dos bytes menos significativos
        MBR = *data; //datos
    }
    return 0;
}

/**
 * escribe de 0 a 4 bytes desde la posicion apuntada por puntero_l
 *
 * @param puntero_l puntero logico al primer byte que pisar
 * @param c_bytes cantidad de bytes a escribir
 * @param data valor a escribir
 *
 * @return se retorna el codigo de error de la operacion 0 = sin error -1 = error de segmento
 */
int escribir_memoria(uint32_t puntero_l, uint8_t c_bytes, uint32_t data)
{
    int dir_fisica;
    if(!chunk_memoria_valido(puntero_l, c_bytes, &dir_fisica))
        return -1; // error de segmento

    //carga los registros LAR, MAR y MBR
    //siempre se van a cargar los registros cuando se escriba en la memoria y que toda escritura es de datos
    LAR = puntero_l; //direccion logica
    MAR = ((uint32_t)c_bytes << 16) | ((uint32_t)dir_fisica & 0xFFFF); //cantidad de bytes en los dos bytes mas significativos
                                                                        //y direccion fisica en los dos bytes menos significativos
    MBR = data; //datos

    for(int i=0; i<c_bytes; i++)
    {
        uint8_t shift_bytes = c_bytes-1-i; //   c_bytes-1 = ultimo byte   =>   c_bytes-2   =>   ...   =>   c_bytes-c_bytes = 0 shift = primer byte
        uint8_t shift_bits = 8*shift_bytes;
        uint8_t byte = data>>shift_bits;
        memoria[dir_fisica+i] = byte;
    }

    return 0;
}

/**
 * @return retorna codigo de error (estaba que 0 es error y 1 es todo ok, pero haciendo que 0 sea todo ok podemos tener multiples codigos de error)
 * 0 = OK
 * -1 = Error de segmento
 * -2 = Instruccion invalida
 * -3 = Division por cero
 *
 * se pueden emitir otros errores que son por razones no previstas, o que no son las 3 standar que nos dan
 */
int lectura_programa(){
    uint8_t tipo_p1 = 0;
    uint8_t tipo_p2 = 0;
    uint32_t data_p1 = 0;
    uint32_t data_p2 = 0;
    uint32_t tam_instruccion = 1;

    uint32_t operacion; // tube que hacerlo 32 en vez de 8 para poder pasarlo como parametro uint32_t* de leer_memoria :/
    if(leer_memoria(IP, 1, &operacion, 1))
        return -1; // error de segmento

    OPC  = operacion & 0b00011111;
    tipo_p2 = (operacion >> 6) & 0b00000011;            // Bits 7 y 6: Operando B
    tipo_p1 = ((operacion >> 4) & 0b00000011);          // Operando A

    uint32_t pl_p2 = IP+1;
    uint32_t pl_p1 = IP+1+tipo_p2;

    if(
        leer_memoria(pl_p2, tipo_p2, &data_p2, 1) ||
        leer_memoria(pl_p1, tipo_p1, &data_p1, 1)
    )
        return -1; // error de segmento

    tam_instruccion += (tipo_p1 + tipo_p2);

    if(!tipo_p1)
    {
        tipo_p1 = tipo_p2;
        tipo_p2 = 0;
        data_p1 = data_p2;
        data_p2 = 0;
    }


    OP1 = tipo_p1<<24;
    OP1 += data_p1;
    OP2 = tipo_p2<<24;
    OP2 += data_p2;
    IP += tam_instruccion;// desplazo IP a la siguiente instruccion

    //printf("IP %08X  OPC %08X  OP1 %08X  OP2 %08X\n", IP, OPC, OP1, OP2);

    uint8_t index_c = OPC;
    int err;
    if(tipo_p1==0 && tipo_p2==0) // 0 params (STOP)
    {
        if(index_c != 0x0F) // si tiene 0 parametros y no es STOP, Instruccion invalida
            return -2; // Instruccion invalida
        err = opc_stop();
    }
    else if(tipo_p2==0) // 1 param
    {
        if(index_c<0 || index_c>0x0A)
            return -2; // Instruccion invalida
        err = operaciones_1_param[index_c](OP1);
    }
    else // 2 params
    {
        index_c -= 0x10;
        if(index_c<0 || index_c>0x0F)
            return -2; // Instruccion invalida
        err = operaciones_2_params[index_c](OP1, OP2);
    }

    char op1_str[15];
    char op2_str[15];
    op_a_str(OP1, op1_str);
    op_a_str(OP2, op2_str);


    /*printf("\noperacion: %02X\n", operacion); // out de debug para tantear los valores leidos
    printf("tipo op:   %s (%02X)\n", mnemonicos[OPC], OPC);
    printf("op1:       %s (%06X)\n", op1_str, OP1);
    printf("op2:       %s (%06X)\n", op2_str, OP2);*/
    printf("instruccion disassembler: %4s %s %s\n", mnemonicos[OPC], op1_str, op2_str);
    printf("err code:  %d\n\n", err);

    return err;
}

int get_dato_op(uint32_t op, int32_t* dato)
{
    if(!dato)
    {
        printf("ERROR: get_dato_op() recibio dato=NULL!!!\n");
        return -10;
    }

    uint8_t tipo = (op>>24)&0x3;
    uint8_t index_reg;
    switch(tipo){
        case 0: // no hay operando
            *dato = 0;
            break;
        case 1: // operando de registro
            index_reg = op&0x0000001F;
            *dato = registros[index_reg];
            break;
        case 2: // operando inmediato
            *dato = op&0x0000FFFF;
            if(op&0x00008000) // si el ultimo bit de los 2 bytes de informacion es un 1, el numero es negativo, se rellenan los restantes bits con 1s
                op += 0xFFFF0000;
            break;
        case 3: // operando de memoria
            index_reg = op&0x0000001F;
            int16_t extra_offset = (op>>8)&0x0000FFFF;

            uint32_t l_pointer = registros[index_reg]+extra_offset;
            if(leer_memoria(l_pointer, 4, dato, 0))
                return -1;
            break;
        //default: no hace falta porque tipo se pasa por una mascara de 2 bits, osea que no hay otro valor posible aparte de 0 1 2 3
    }
    return 0;
}

int set_dato_op(uint32_t op, int32_t dato)
{
    uint8_t tipo = (op>>24)&0x3;
    uint8_t index_reg;
    switch(tipo){
        case 0: // no hay operando
            return -20; // tipo de operando izquierdo invalido
        case 1: // operando de registro
            index_reg = op & 0x0000001F; // mascara para los ultimos 5 bits
            registros[index_reg] = dato;
            break;
        case 2: // operando inmediato
            return -20; // tipo de operando izquierdo invalido
        case 3: // operando de memoria
            index_reg = op&0x0000001F;
            int16_t extra_offset = (op>>8)&0x0000FFFF;

            uint32_t l_pointer = registros[index_reg]+extra_offset;
            if(escribir_memoria(l_pointer, 4, dato))
                return -1;
            break;
        //default: no hace falta porque tipo se pasa por una mascara de 2 bits, osea que no hay otro valor posible aparte de 0 1 2 3
    }
    return 0;
}

int opc_stop()
{
    IP = 0xFFFFFFFF;
    return 0;
}

void set_flags(int n, int z, int c, int v) {
    uint32_t cc = 0;
    if (n)
        cc = cc | FLAG_N;
    if (z)
        cc = cc | FLAG_Z;
    if (c)
        cc = cc | FLAG_C;
    if (v)
        cc = cc | FLAG_V;
    CC = cc; //registro 17 es el CC, lleva en 1 en los 4 bits mas significativos si se activa alguna flag (red flag)
}

int opc_mov(uint32_t op1, uint32_t op2)
{
    uint32_t dato_op2;
    int err = get_dato_op(op2, &dato_op2);
    if(err)
        return err;

    set_flags((int32_t)dato_op2 < 0, dato_op2 == 0, 0, 0); //carga en CC si es cero o negativo
    err = set_dato_op(op1, dato_op2);
    if(err)
        return err;
    return 0;
}

int opc_add(uint32_t op1, uint32_t op2)
{
    int32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if (err)
        return err;

    err = get_dato_op(op2, &b);
    if (err)
        return err;

    res = a + b;

    // Flags
    int n = res < 0; // resultado negativo
    int z = (res == 0); //resultado igual a cero
    int c = (res < a); // acarreo en suma sin signo
    // Overflow con signo: si signos iguales dan signo opuesto
    int v = (((a ^ res) & (b ^ res) & 0x80000000U) != 0);

    set_flags(n, z, c, v);

    err = set_dato_op(op1, res);
    if (err)
        return err;

    return 0;
}

int opc_sub(uint32_t op1, uint32_t op2)
{
    int32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if (err)
        return err;

    err = get_dato_op(op2, &b);
    if (err)
        return err;

    res = a - b;

    // Flags
    int n = res < 0; // resultado negativo
    int z = (res == 0); //resultado igual a cerop
    int c = (a < b); // si a es menor a b, el numero es negativo
    // Overflow con signo: si signos iguales dan signo opuesto
    int v = (((a ^ b) & (a ^ res) & 0x80000000U) != 0);

    set_flags(n, z, c, v);

    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_mul(uint32_t op1, uint32_t op2)
{
    int32_t d1, d2;
    int err = get_dato_op(op1, &d1);
    if (err)
        return err;
    err = get_dato_op(op2, &d2);
    if (err)
        return err;

    int64_t a = d1, b = d2, res; // esto para que res pueda tomar valores fuera de los limites de 32 bits, y si sucede, se detecta y se setea el carry de cc
    res = a * b;

    // Flags
    int n = res < 0; // resultado negativo
    int z = res == 0; //resultado igual a cerop
    int c = res > 2147483846 || res < -2147483846; // acarreo en la multiplicacion, si hay acarreo tambien hay overflow
    int v = c;

    set_flags(n, z, c, v);

    err = set_dato_op(op1, (int32_t)res);
    if (err)
        return err;

    return 0;
}

int opc_div(uint32_t op1, uint32_t op2)
{
    int32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if (err)
        return err;
    err = get_dato_op(op2, &b);
    if (err)
        return err;

    if (b==0)
        return -1; //no se puede dividir por cero

    res = a / b;


    if(a<0 && b>0) // -- proceso para hacer que la vision siempre concluya con resto positivo (medio dificil de entender viendolo directamente, cualquier cosa preguntenme. atte gaspar)
        res--;
    else if(a<0 && b<0)
        res++;
    AC = a-res*b; // se le asigna a AC el resto de la division


    // Flags
    int n = res < 0; // resultado negativo

    // Caso especial de overflow en división con signo: INT32_MIN / -1
    if (a == -2147483648 && b == -1) {
        int32_t res = (int32_t)a;
        set_flags(n,0,1,1);
    } else {
        set_flags(n, 0, 0, 0);
    }

    err = set_dato_op(op1, res);
    if (err)
        return err;

    return 0;
}

int opc_and(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if(err)
        return err;

    err = get_dato_op(op2,&b);
    if(err)
        return err;

    res = a & b;
    set_flags((int32_t)res < 0, res == 0, 0, 0); //carga en CC si es cero o negativo
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_or(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if(err)
        return err;

    err = get_dato_op(op2,&b);
    if(err)
        return err;

    res = a | b;
    set_flags((int32_t)res < 0, res == 0, 0, 0); //carga en CC si es cero o negativo
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_xor(uint32_t op1, uint32_t op2)
{
    uint32_t a,b,res;
    int err = get_dato_op(op1, &a);
    if(err)
        return err;

    err = get_dato_op(op2,&b);
    if(err)
        return err;

    res = a ^ b;
    set_flags((int32_t)res < 0, res == 0, 0, 0); //carga en CC si es cero o negativo
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_swap(uint32_t op1, uint32_t op2)
{
    uint32_t a,b,res;
    uint8_t tipo1 = (op1>>24)&0x3;
    uint8_t tipo2 = (op2>>24)&0x3;
    if ((tipo1==1 || tipo1==3) && (tipo2==1 || tipo2==3)) { //solo se puede aplicar la operacion en registros y/o celdas
        int err = get_dato_op(op1, &a);
        if(err)
            return err;

        err = get_dato_op(op2,&b);
        if(err)
            return err;

        a = a ^ b;
        b = b ^ a;
        res = a ^ b;
        set_flags((int32_t)res < 0, res == 0, 0, 0); //carga en CC si es cero o negativo
        err = set_dato_op(op1, res);
        err = set_dato_op(op2, b);
        if(err)
            return err;
    }
    return 0;
}

int opc_shl(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    int err, n, z, c, v;
    err = get_dato_op(op1, &a);
    if(err)
        return err;
    err = get_dato_op(op2,&b);
    if(err)
        return err;

    c=0;
    v=0;
    if (b == 0) {
        res = a;
        n=0;
        z=0;
    } else
        if (b < 32) {
            c = (int)(a >> 32 - b) & 1; //toma el valor del ultimo bit que sale afuera, sino hay carry toma cero
            res = a << b;
            v = (((a ^ res) & 0x80000000) != 0); //hay desbordamiento si el bit 31 difiere de a o si algún bit expulsado era distinto del bit de signo
        } else
            if (b == 32) { //caso especifico para cuando a es 1 o 0
                c = (int)(a & 1U);
                res = 0;
                v = (a != 0);
            } else {
                c = 0;
                res = 0;
                v = (a != 0);
            }

    n = ((int32_t)res < 0);
    z = (res == 0);
    set_flags(n,z,c,v); //carga en CC
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_shr(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    int err, n, z, c, v;
    err = get_dato_op(op1, &a);
    if(err)
        return err;
    err = get_dato_op(op2,&b);
    if(err)
        return err;
    //aun tengo algunas dudas del bit de acarreo pero por lo que vi en este tipo de casos se puede activar
    c=0;
    v=0; //imposible que sea distinto de cero
    if (b == 0) {
        res = a;
    } else
        if (b < 32) {
            // El último bit expulsado por la derecha estaba en la posición (b - 1)
            c = (int)((a >> (b - 1)) & 1);
            res = a >> b;
        } else
            if (b == 32) {
                c = (int)((a >> 31) & 1U);
                res = 0;
            } else {
                res = 0;
            }
    n = ((int32_t)res < 0);
    z = (res == 0);
    set_flags(n,z,c,v); //carga en CC
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_sar(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    int32_t castA,castRes;
    int err, n, z, c, v;
    err = get_dato_op(op1, &a);
    if(err)
        return err;
    err = get_dato_op(op2,&b);
    if(err)
        return err;
    castA = (int32_t)a; //cast para enteros con signo
    c=0;
    v=0; //imposible que sea distinto de cero
    if (b == 0) {
        castRes=castA;
    } else
        if (b < 32) {
            c = (int)((a >> (b - 1)) & 1U);
            castRes = castA >> b;
        } else {
            c = castA < 0; //si es negativo va a haber carry por que se llena todo de unos
            castRes = (castA<0) ? -1 : 0;
        }
    res=(uint32_t)castRes;
    n = castRes < 0;
    z = res == 0;
    set_flags(n,z,c,v); //carga en CC
    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_rnd(uint32_t op1, uint32_t op2)
{
    uint32_t tope;
    int err = get_dato_op(op2, &tope);
    if (err)
        return err;

    int32_t limite = (int32_t)tope; //cast para que pueda ser negativo tambien
    uint32_t aleatorio = 0;

    if (limite > 0) {
        aleatorio = (uint32_t)(rand() % (limite + 1));
    } else if (limite < 0) {
        // Por si op2 fuera negativo: genera entre [limite, 0]
        aleatorio = (uint32_t)(-(rand() % (-limite + 1)));
    } else {
        aleatorio = 0;
    }

    err = set_dato_op(op1, aleatorio);
    if(err)
        return err;
    return 0;
}

int opc_ldl(uint32_t op1, uint32_t op2)
{
    uint32_t dato_op1;
    int err = get_dato_op(op1, &dato_op1);
    if(err)
        return err;

    uint32_t dato_op2;
    err = get_dato_op(op2, &dato_op2);
    if(err)
        return err;

    dato_op1 &= 0xFFFF0000;
    dato_op2 &= 0x0000FFFF;
    dato_op1 |= dato_op2;

    err = set_dato_op(op1, dato_op1);
    if(err)
        return err;

    return 0;
}

int opc_ldh(uint32_t op1, uint32_t op2)
{
    uint32_t dato_op1;
    int err = get_dato_op(op1, &dato_op1);
    if(err)
        return err;

    uint32_t dato_op2;
    err = get_dato_op(op2, &dato_op2);
    if(err)
        return err;


    dato_op1 &= 0x0000FFFF;
    dato_op2 &= 0x0000FFFF; // dejo los ultimos 2 bytes
    dato_op2 <<= 16; //        los muevo a la parte alta
    dato_op1 |= dato_op2;


    err = set_dato_op(op1, dato_op1);
    if(err)
        return err;

    return 0;
}


int opc_sys(uint32_t op1)
{
    uint32_t dato_op1;
    int err = get_dato_op(op1, &dato_op1);
    if(err)
        return err;
    dato_op1 &= 0x0000001F; // dejo solo los ultimos 5 bits


    uint16_t c_vals = ECX;
    uint16_t tam_vals = ECX>>16;
    uint32_t puntero_l = EDX;
    int dir_puntero;

    int index_format=0;
    uint8_t bit_mask = 0x00000001;
    for(index_format=0; index_format<5 && !(EAX&bit_mask); index_format++)
        bit_mask <<= 1;

    if(index_format>4){
        printf("ERROR ejecutando SYS, EAX no tiene un valor valido\n");
        return -10;
    }
    switch(dato_op1){
        case 1:
            // -- seccion pendiente de cambio, bastane fea y seguro se pueda hacer algo que funcione para el SYS 1 y SYS 2
            // --

            for(int i=0; i<c_vals; i++)
            {
                if(puntero_logico_a_direccion_fisica(puntero_l, &dir_puntero)) return -1;
                printf("[%04X]: ", dir_puntero);

                uint32_t input=0;

                if(index_format!=4) // binario es mas raro
                {

                    char scanf_format[3] = "% ";
                    scanf_format[1] = formatos_sys[index_format]; // relleno el espacio en scanf_format con el formato del input
                    if (index_format == 1) { // Caracter individual
                        scanf(" %c", (char*)&input);
                    } else {
                        scanf(scanf_format, &input);
                    }
                }
                else // formato binario, incomodo
                {
                    char b_input[33];
                    scanf("%32s", b_input);
                    int bits = strlen(b_input);
                    for(int j=0; j<bits; j++)
                    {
                        input <<= 1; // creo un espacio para el sig bit
                        if(b_input[j]!='0')
                            input |= 1; // si es 1, lo relleno con 1
                    }
                }

                if(escribir_memoria(puntero_l, tam_vals, input))
                    return -1;
                puntero_l+=tam_vals;
            }
            break;
        case 2:
            // MODO REGISTRO DIRECTO: si tam_vals o c_vals es 0, imprime directamente EDX
            if(tam_vals == 0 || c_vals == 0) {
                if(index_format != 4) {
                    char printf_format[4] = "%";
                    printf_format[1] = formatos_sys[index_format];
                    if (index_format == 0) {
                        printf(printf_format, (int32_t)EDX);
                    } else {
                        printf(printf_format, EDX);
                    }
                } else { // binario
                    for(int b = 31; b >= 0; b--)
                        printf("%d", (EDX >> b) & 1);
                }
                if(index_format != 1) printf("\n");
                break;
            }
            // MODO MEMORIA: lee c_vals elementos desde puntero_l
            char printf_format[4] = "% \0";
            if(index_format != 4)
                printf_format[1] = formatos_sys[index_format];

            for(int i = 0; i < c_vals; i++)
            {
                uint32_t dato = 0;
                if(leer_memoria(puntero_l, tam_vals, &dato, 0))
                    return -1;

                if(index_format != 4)
                {
                    // Extensión de signo si es decimal y menor a 4 bytes
                    if(index_format == 0) {
                        if(tam_vals == 1) dato = (uint32_t)(int32_t)(int8_t)dato;
                        else if(tam_vals == 2) dato = (uint32_t)(int32_t)(int16_t)dato;
                        printf(printf_format, (int32_t)dato);
                    } else {
                        printf(printf_format, dato);
                    }
                }
                else // binario a mano para memoria
                {
                    int total_bits = (tam_vals <= 4) ? (tam_vals * 8) : 32;
                    for(int b = total_bits - 1; b >= 0; b--)
                        printf("%d", (dato >> b) & 1);
                }

                // Espacio entre elementos si no es texto plano (%c)
                if(index_format != 1 && i + 1 < c_vals)
                    printf(" ");

                puntero_l += tam_vals;
            }
            if(index_format != 1) printf("\n");
            break;
        default:
            printf("ERROR SYS recibio un valor que no es 1 ni 2\n");
            return -10;
    }
    return 0;
}



int opc_not(uint32_t op1){

    int32_t val;
    int err = get_dato_op(op1, &val);
    if (err) return err;

    uint32_t res = ~((uint32_t)val);
    int n = ((int32_t)res < 0);
    int z = (res == 0);
    set_flags(n, z, 0, 0);

    return set_dato_op(op1, (int32_t)res);
}

int opc_jmp(uint32_t op1)
{
    int32_t destino;
    int err = get_dato_op(op1, &destino);
    if (err)
        return err;

    IP = CS | ((uint32_t)destino & 0xFFFF); //por si en el futuro CS no esta en el segmento 0
    return 0;
}
int opc_jp(uint32_t op1)  { // Positivo estricto
    if( (CC & FLAG_N) == 0 && (CC & FLAG_Z) == 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jn(uint32_t op1){ // Negativo
    if( (CC & FLAG_N) != 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jz(uint32_t op1)  { //Cero
    if( (CC & FLAG_Z) != 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jc(uint32_t op1)  { //Carry
    if( (CC & FLAG_C) != 0)
        return opc_jmp(op1);
    else
        return 0;
} // Carry
int opc_jv(uint32_t op1)  { //Overflow
    if( (CC & FLAG_V) != 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jnp(uint32_t op1) { // No positivo (<= 0)
    if( (CC & FLAG_N) != 0 || (CC & FLAG_Z) != 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jnn(uint32_t op1) { // No negativo (>= 0)
    if( (CC & FLAG_N) == 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_jnz(uint32_t op1) {
    if( (CC & FLAG_Z) == 0)
        return opc_jmp(op1);
    else
        return 0;
}
int opc_cmp(uint32_t op1, uint32_t op2){
    int32_t a, b, res;
    int err = get_dato_op(op1, &a);
    if (err)
        return err;
    err = get_dato_op(op2, &b);
    if (err)
        return err;
    res = a - b;
    int n = (res < 0);
    int z = (res == 0);
    int c = (a < b); //al calcularse como SUB, si la resta da un resultado negativo afecta al carry
    int v = (a >= 0 && b < 0 && res < 0) || (a < 0 && b >= 0 && res >= 0);

    set_flags(n, z, c, v);
    return 0;
}

/**
 * genera el string representativo del operando en assembler
 * 
 * @param op operando, como esta en OP1 y OP2
 * @param str puntero a un bloque de memoria de bits suficientes, con 15 bytes es suficiente
 * 
 */
void op_a_str(uint32_t op, char* str)
{
    uint8_t tipo = (op>>24)&0b11;
    uint32_t data = op&0x00FFFFFF;
    if(tipo==0)
        strcpy(str, "");
    else if(tipo==1)
        strcpy(str, registros_nombres[data]);
    else if(tipo==2)
    {
        int16_t numero = data;

        sprintf(str, "%d", numero);
    }
    else
    {
        int16_t offset = data>>8;
        uint8_t registro = data&0x1F;

        sprintf(str, "[%s", registros_nombres[registro]);
        char aux[15];
        if(offset)
        {
            strcpy(aux, str);
            if(offset>0)
                sprintf(str, "%s+%d", aux, offset);
            else
                sprintf(str, "%s%d", aux, offset);
        }
        strcpy(aux, str);
        sprintf(str, "%s]", aux);
    }
}
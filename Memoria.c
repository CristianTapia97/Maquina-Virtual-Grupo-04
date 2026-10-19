#include "Tipos.h"
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
        return ERR_FS;
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
        return ERR_FS; // error de segmento

    *data = 0;
    for(int i=0; i<c_bytes; i++)
    {
        *data <<= 8;
        *data += memoria[dir_fisica+i];
    }
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
        return ERR_FS; // error de segmento

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

    LAR = puntero_l; //direccion logica
    MAR = ((uint32_t)c_bytes << 16) | ((uint32_t)dir_fisica & 0xFFFF); //cantidad de bytes en los dos bytes mas significativos
                                                                       //y direccion fisica en los dos bytes menos significativos
    MBR = data; //datos

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
    
    if (leer_memoria(IP, 1, &operacion, 1))
        return ERR_FP; // fin de programa

    OPC  = operacion & 0b00011111;
    tipo_p2 = (operacion >> 6) & 0b00000011;            // Bits 7 y 6: Operando B
    tipo_p1 = ((operacion >> 4) & 0b00000011);          // Operando A

    uint32_t pl_p2 = IP+1;
    uint32_t pl_p1 = IP+1+tipo_p2;

    if(
        leer_memoria(pl_p2, tipo_p2, &data_p2, 1) ||
        leer_memoria(pl_p1, tipo_p1, &data_p1, 1)
    )
        return ERR_FP; // fin de programa

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
            return ERR_FS; // Instruccion invalida
        err = opc_stop();
    }
    else if(tipo_p2==0) // 1 param
    {
        if(index_c<0 || index_c>0x0A)
            return ERR_FS; // Instruccion invalida
        err = operaciones_1_param[index_c](OP1);
    }
    else // 2 params
    {
        index_c -= 0x10;
        if(index_c<0 || index_c>0x0F)
            return ERR_FS; // Instruccion invalida
        err = operaciones_2_params[index_c](OP1, OP2);
    }

    char op1_str[15];
    char op2_str[15];
    op_a_str(OP1, op1_str);
    op_a_str(OP2, op2_str);

    /*
    printf("\noperacion: %02X\n", operacion); // out de debug para tantear los valores leidos
    printf("tipo op:   %s (%02X)\n", mnemonicos[OPC], OPC);
    printf("op1:       %s (%06X)\n", op1_str, OP1);
    printf("op2:       %s (%06X)\n", op2_str, OP2);
    printf("instruccion disassembler: %4s %s %s\n", mnemonicos[OPC], op1_str, op2_str);
    printf("err code:  %d\n\n", err);
*/
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
                *dato += 0xFFFF0000;
            break;
        case 3: // operando de memoria
            index_reg = op&0x0000001F;
            int16_t extra_offset = (op>>8)&0x0000FFFF;

            uint32_t l_pointer = registros[index_reg]+extra_offset;
            if(leer_memoria(l_pointer, 4, dato, 0))
                return ERR_FS;
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
                return ERR_FS;
            break;
        //default: no hace falta porque tipo se pasa por una mascara de 2 bits, osea que no hay otro valor posible aparte de 0 1 2 3
    }
    return 0;
}
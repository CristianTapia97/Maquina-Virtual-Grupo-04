#include "Tipos.h"
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
    if(tipo==0) // sin param
        strcpy(str, "");
    else if(tipo==1){ // registro
        
        uint8_t seccion = (data&0b01100000)>>5;
        uint8_t registro = data&0b00011111;
        strcpy(str, registros_nombres[registro]);
        if(seccion==0b11 || seccion==0b01 || seccion==0b10) {
            strcpy(str, str+1); // str+1 hace que arranque a copiar el registro salteandose el primer caracter:   EAX => AX
            
            if(seccion==0b01)
                str[1] = 'L';
            else if(seccion==0b10)
                str[1] = 'H';
        }
        
    }
    else if(tipo==2) // inmediato
    {
        int16_t numero = data;

        sprintf(str, "%d", numero);
    }
    else // registro
    {
        int16_t offset = data>>8;
        uint8_t registro = data&0b00011111;
        uint8_t tamanio = (data&0b11100000)>>5;
        char c_tamanio;
        if(tamanio==0b001)
            c_tamanio = 'b';
        else if(tamanio==0b010)
            c_tamanio = 'w';
        else
            c_tamanio = 'l';


        sprintf(str, "%c[%s", c_tamanio, registros_nombres[registro]);
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

int disassembler()
{
    int err;

    if (KS != 0xFFFFFFFF && KS != -1)
    {
        uint32_t id_ks = KS >> 16;
        uint32_t entrada_ks = tabla_segmentos[id_ks];
        uint32_t dir_ks = entrada_ks >> 16;
        uint32_t tam_ks = entrada_ks & 0x0000FFFF;

        uint32_t offset = 0;
        while (offset < tam_ks)
        {
            uint32_t dir_fisica = dir_ks + offset;
            uint32_t inicio = offset;

            while (offset < tam_ks && memoria[dir_ks + offset] != '\0')
                offset++;
            if (offset < tam_ks && memoria[dir_ks + offset] == '\0')
                offset++;

            uint32_t len_total = offset - inicio;

            printf(" [%04X] ", dir_fisica);

            if (len_total > 7)
            {
                for (int i = 0; i < 6; i++)
                    printf("%02X ", (uint8_t)memoria[dir_ks + inicio + i]);
                printf(".. | \"");
            }
            else
            {
                for (uint32_t i = 0; i < len_total; i++)
                    printf("%02X ", (uint8_t)memoria[dir_ks + inicio + i]);
                for (uint32_t i = len_total; i < 7; i++)
                    printf("   ");
                printf("| \"");
            }

            for (uint32_t i = 0; i < len_total - 1; i++)
            {
                unsigned char c = memoria[dir_ks + inicio + i];
                if (c >= 32 && c <= 126)
                    putchar(c);
                else
                    putchar('.');
            }
            printf("\"\n");
        }
    }

    uint32_t ip = CS;
    uint32_t opc;
    uint32_t op1;
    uint32_t op2;
    char op1_str[15];
    char op2_str[15];
    uint8_t tipo_p1 = 0;
    uint8_t tipo_p2 = 0;
    uint32_t data_p1 = 0;
    uint32_t data_p2 = 0;
    uint32_t tam_instruccion = 1;
    uint32_t operacion;
    uint32_t dir;
    uint8_t byte;

    

    while(chunk_memoria_valido(ip, 1, NULL))
    {
        tam_instruccion = 1;

        if(leer_memoria(ip, 1, &operacion, 1))
            return ERR_FS; // error de segmento

        opc  = operacion & 0b00011111;
        tipo_p2 = (operacion >> 6) & 0b00000011;            // Bits 7 y 6: Operando B
        tipo_p1 = ((operacion >> 4) & 0b00000011);          // Operando A

        uint32_t pl_p2 = ip+1;
        uint32_t pl_p1 = ip+1+tipo_p2;

        if(
            leer_memoria(pl_p2, tipo_p2, &data_p2, 1) ||
            leer_memoria(pl_p1, tipo_p1, &data_p1, 1)
        )
            return ERR_FS; // error de segmento

        tam_instruccion += (tipo_p1 + tipo_p2);

        if(!tipo_p1)
        {
            tipo_p1 = tipo_p2;
            tipo_p2 = 0;
            data_p1 = data_p2;
            data_p2 = 0;
        }


        op1 = tipo_p1<<24;
        op1 += data_p1;
        op2 = tipo_p2<<24;
        op2 += data_p2;
        
        
        if(err = puntero_logico_a_direccion_fisica(ip, &dir))
            return err;
        
        uint32_t offset_instruccion = ip & 0x0000FFFF;
        if(offset_instruccion == (entry_point & 0x0000FFFF))
            printf(">[%04X] %02X", dir, operacion);
        else
            printf(" [%04X] %02X", dir, operacion);

        ip += tam_instruccion;// desplazo IP a la siguiente instruccion
        op_a_str(op1, op1_str);
        op_a_str(op2, op2_str);

        for(int i=tipo_p2-1; i>=0; i--)
        {
            byte = data_p2>>(i*8);
            printf(" %02X", byte);
        }


        for(int i=tipo_p1-1; i>=0; i--)
        {
            byte = data_p1>>(i*8);
            printf(" %02X", byte);
        }

        int c_espacios = (6-tipo_p1-tipo_p2)*3;
        for(int i=0; i<c_espacios; i++)
            printf(" ");

        
        printf(" | ");

        printf("%-4s ", mnemonicos[opc]);
        if(tipo_p1)
            printf("%s", op1_str);
        if(tipo_p2)
            printf(", %s", op2_str);
        printf("\n");
    }

    return 0;
}
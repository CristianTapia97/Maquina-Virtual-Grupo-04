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

    uint32_t charp = KS; // char pointer para los caracteres en el const segment
    while(chunk_memoria_valido(charp, 1, NULL))
    {
        if(err = puntero_logico_a_direccion_fisica(charp, &dir))
            return err;
        printf("[%04X] ",dir);
        
        int bytes_mostrados = 0;
        for(int i=0; i<6; i++)
        {
            if(err = puntero_logico_a_direccion_fisica(charp+i, &dir))
                return err;
            char c = memoria[dir];
            printf("%02X ", c);
            bytes_mostrados++;
            if(c=='\0')
                break;
        }

        if(memoria[dir]!='\0')  // si corto por mostrar 6 caracteres y no por haber encontrado el \0, completo el ultimo byte con \0 o .. correspondientemente
            if(memoria[dir+1]!='\0')
                printf(".. ");
            else
                printf("00 ");
        else                    // si corto habiendo encontrado el final del texto, tengo que completar bytes con espacios
            for(int i=0; i<7-bytes_mostrados; i++)
                printf("   ");

        printf("| ");

        printf("\"");

        if(err = puntero_logico_a_direccion_fisica(charp, &dir))
            return err;
        while(memoria[dir]!='\0')
        {
            if(32<=memoria[dir] && memoria[dir]<=126)
                printf("%c", memoria[dir]);
            else
                printf(".");
            charp++;
            if(err = puntero_logico_a_direccion_fisica(charp, &dir))
                return err;
        }
        printf("\"\n");
        charp++; // avanzo al siguiente texto
    }

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
        
        ip += tam_instruccion;// desplazo IP a la siguiente instruccion
        op_a_str(op1, op1_str);
        op_a_str(op2, op2_str);

        printf("[%04X] %02X",dir, operacion);

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
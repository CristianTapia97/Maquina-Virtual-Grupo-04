#include "Tipos.h"
#include <stdio.h>
// si se usa despues de un scanf resuelve problemas con que quedan \n al apretar enter y el siguiente scanf agarra el \n en vez de un nuevo input
void limpiar_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF); // magia negra de la IA
}
int opc_stop()
{
    IP = 0xFFFFFFFF;
    return 0;
}
int opc_sys(uint32_t op1)
{
    uint32_t dato_op1;
    int index_format;
    int err = get_dato_op(op1, &dato_op1);
    if(err)
        return err;


    uint16_t c_vals = ECX;
    uint16_t tam_vals = ECX>>16;
    uint32_t puntero_l = EDX;
    int dir_puntero;


    uint8_t bit_mask = 0x00000001;


    switch(dato_op1){
        case 1:
            for(index_format=0; index_format<5 && !(EAX&bit_mask); index_format++) // mecanismo para conseguir el primer formato, y solo se usara ese
                bit_mask <<= 1;
            if(index_format>4){
                printf("ERROR ejecutando SYS, EAX no tiene un valor valido\n");
                return -10;
            }

            for(int i=0; i<c_vals; i++)
            {
                if(puntero_logico_a_direccion_fisica(puntero_l, &dir_puntero)) return ERR_FS;
                printf("[%04X]: ", dir_puntero);

                int32_t input=0;

                if(index_format!=4) // binario es mas raro
                {
                    scanf(formatos_sys[index_format], &input);
                    limpiar_input_buffer();
                }
                else // formato binario, incomodo
                {
                    char b_input[33];
                    scanf("%32s", b_input);
                    limpiar_input_buffer();
                    int bits = strlen(b_input);
                    for(int j=0; j<bits; j++)
                    {
                        input <<= 1; // creo un espacio para el sig bit
                        if(b_input[j]!='0')
                            input |= 1; // si es 1, lo relleno con 1
                    }
                }

                if(escribir_memoria(puntero_l, tam_vals, input))
                    return ERR_FS;
                puntero_l+=tam_vals;
            }
            break;
        case 2:
            /*// MODO REGISTRO DIRECTO: si tam_vals o c_vals es 0, imprime directamente EDX
            if(tam_vals == 0 || c_vals == 0) {
                if(index_format != 4) {
                    char printf_format[4] = "% ";
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
            }*/
            // MODO MEMORIA: lee c_vals elementos desde puntero_l

            for(int i = 0; i < c_vals; i++)
            {
                if(puntero_logico_a_direccion_fisica(puntero_l, &dir_puntero)) return ERR_FS;
                printf("[%04X]: ", dir_puntero);

                uint32_t dato = 0, dato_int;
                if(leer_memoria(puntero_l, tam_vals, &dato, 0))
                    return ERR_FS;

                //for(index_format=0; index_format<5 && !(EAX&bit_mask); index_format++) // mecanismo para conseguir el primer formato, y solo se usara ese
                bit_mask=0x1;
                for(int index_format=0; index_format<5; index_format++)
                {
                    if(EAX&bit_mask) // si la bitmask coincide con un 1 en EAX, se debe mostrar el dato en formato index_format,  realmente seria mejor usar EAX&bit_mask<<index_format en vez de hacer bit_mask<<=1 pero es medio raro de leer
                    {
                        if(index_format != 4)
                        {
                            // Extensión de signo si es decimal y menor a 4 bytes
                            if(index_format == 0) {
                                dato_int = dato;
                                // dato_int en vez de dato porque modifica el dato para los siguientes formatos
                                if(tam_vals == 1) dato_int = (uint32_t)(int32_t)(int8_t)dato;
                                else if(tam_vals == 2) dato_int = (uint32_t)(int32_t)(int16_t)dato;
                                else if(tam_vals == 3) dato_int = (dato&0x00800000)?dato|0xFF000000:dato; // si el tam es de 3, me fijo si el ultimo bit de los 3 bytes es 1, si lo es, relleno con unos
                                printf(formatos_sys[index_format], (int32_t)dato_int);
                            } else {
                                printf(formatos_sys[index_format], dato);
                            }
                        }
                        else // binario a mano para memoria
                        {
                            int total_bits = tam_vals * 8;
                            printf("0b");
                            for(int b = total_bits - 1; b >= 0; b--)
                                printf("%d", (dato >> b) & 1);
                        }
                    }
                    bit_mask<<=1;
                    printf(" ");
                }



                printf("\n");

                puntero_l += tam_vals;
            }
            break;
        default:
            printf("ERROR SYS recibio un valor que no es 1 ni 2, no e\n");
            //return -10;   
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
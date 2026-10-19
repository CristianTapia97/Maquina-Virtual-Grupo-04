#pragma once
#include <stdint.h>
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

#define ERR_FP 1  //"error" fin de programa
#define ERR_FS -1 // error fallo de segmento
#define ERR_II -2 // error instruccion invalida
#define ERR_DC -3 // error division por cero

// Memoria y Registros
uint8_t memoria[SIZE];
int32_t registros[32] = {0};
int32_t tabla_segmentos[8];
// Tabla de segmentos: 8 entradas de 32 bits

char formatos_sys[][6] = { "\%d", "\%c", "0o\%o", "0x\%X"/*, 'b' binario se implementa a mano*/ };

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
int get_dato_op(uint32_t op, int32_t* dato);
int set_dato_op(uint32_t op, int32_t dato);
int lectura_programa();
int iniciar_programa(const char *ruta_archivo);
int str_termina_con(char* str, char* sufijo);
void op_a_str(uint32_t op, char* str);
int disassembler();
void limpiar_input_buffer();
void proceso_suma(int32_t a, int32_t b, int32_t* r);
void errcode_a_str(int err, char* out);

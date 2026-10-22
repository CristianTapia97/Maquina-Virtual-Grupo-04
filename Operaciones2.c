#include "Tipos.h"
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
    

    proceso_suma(a, b, &res);


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

    proceso_suma(a, -b, &res);

    err = set_dato_op(op1, res);
    if(err)
        return err;

    return 0;
}

int opc_mul(uint32_t op1, uint32_t op2)
{
    int32_t d1, d2, res_out;
    int err = get_dato_op(op1, &d1);
    if (err)
        return err;
    err = get_dato_op(op2, &d2);
    if (err)
        return err;

    int64_t a = d1, b = d2, res; // esto para que res pueda tomar valores fuera de los limites de 32 bits, y si sucede, se detecta y se setea el carry de cc
    res = a * b;
    res_out = (int32_t) res;
    // Flags
    int n = res_out < 0; // resultado negativo
    int z = res_out == 0; //resultado igual a cerop
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
        return ERR_DC; //no se puede dividir por cero

    res = a / b;


    if(a<0 && b>0) // -- proceso para hacer que la vision siempre concluya con resto positivo (medio dificil de entender viendolo directamente, cualquier cosa preguntenme. atte gaspar)
        res--;
    else if(a<0 && b<0)
        res++;
    AC = a-res*b; // se le asigna a AC el resto de la division


    // Flags
    int n = res < 0; // resultado negativo
    int z = res == 0;

    // Caso especial de overflow en división con signo: INT32_MIN / -1
    if (a == -2147483648 && b == -1) {
        int32_t res = (int32_t)a;
        set_flags(n,z,1,1);
    } else {
        set_flags(n, z, 0, 0);
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
        if(err)
            return err;
        err = set_dato_op(op2, b);
        if(err)
            return err;
    }
    return 0;
}

int opc_shl(uint32_t op1, uint32_t op2)
{
    uint32_t a, b, res;
    uint64_t a_l, b_l, res_l;
    int err, n, z, c;
    err = get_dato_op(op1, &a);
    if(err)
        return err;
    err = get_dato_op(op2,&b);
    if(err)
        return err;

    a_l = a;
    b_l = b;

    res = a << b;
    res_l = a_l << b_l;
    c = (uint64_t)res != res_l;
    n = ((int32_t)res < 0);
    z = (res == 0);

    set_flags(n,z,c,c); //carga en CC
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
    
    res = a >> b;

    c=0;
    v=0;
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
    int err, n, z, c, v;
    err = get_dato_op(op1, &a);
    if(err)
        return err;
    err = get_dato_op(op2,&b);
    if(err)
        return err;
    
    res = a >> b;
    if(a&0x80000000)
    {
        uint32_t mask = 0x80000000;
        for(int i=0; i<b; i++)
            res |= mask>>i;
    }

    c=0;
    v=0;
    n = ((int32_t)res < 0);
    z = (res == 0);

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

int opc_cmp(uint32_t op1, uint32_t op2){
    int32_t a, b, res;

    int err = get_dato_op(op1, &a);
    if (err)
        return err;
    err = get_dato_op(op2, &b);
    if (err)
        return err;
    
    proceso_suma(a, -b, &res);
    
    return 0;
}

void proceso_suma(int32_t a, int32_t b, int32_t* r)
{
    uint64_t ua = a;
    uint64_t ub = b;
    int64_t sa = a;
    int64_t sb = b;

    ua &= 0x00000000FFFFFFFF;
    ub &= 0x00000000FFFFFFFF;

    uint64_t ur = ua + ub;
    int64_t sr = sa + sb;
    *r = a + b;

    uint8_t v = (sr > INT32_MAX || sr < INT32_MIN);
    //uint8_t v = *r != (int32_t)ur;
    uint8_t c = ur > 0xFFFFFFFF;
    uint8_t z = *r==0;
    uint8_t n = *r<0;

    set_flags(n,z,c,v);
}
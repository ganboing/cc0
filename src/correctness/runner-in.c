#include "libi0/stdio.h"

register long R1;
register long R2;
register long R3;
register long R4;

long data[1000];

void runner1()
{
    output_char('O');
    output_char('K');
    output_char(C_n);
    commit;
}

void main()
{
    long n;
    long space;
    space = 0x400000000000;

    runner runner1()
        using data[0,,800]
        in space;

    n = 0x44;

    commit;
}



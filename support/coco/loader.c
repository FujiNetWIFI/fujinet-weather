#include <cmoc.h>
#include <coco.h>

void runm(const char *filename)
{
    *((uint16_t *)0x2dd) = 0x4D22;
    strcpy(0x2df, filename);
    *((uint16_t *)0xa6) = 0x2dd;

    asm
    {
        ldd     #$4D1C
        jmp     $AE75
    }
}

int main(void)
{
    initCoCoSupport();
    runm(isCoCo3 ? PRODUCT "3" : PRODUCT "1");
    return 0;
}

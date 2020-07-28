//
// /opt/nec/ve/bin/ncc -shared -fpic -pthread -o libvestackargs.so libvestackargs.c
//

void test_out(int in, void **out)
{
    if (in)
        *out = (void *)0x12345679;
    else
        *out = (void *)0x12345678;
}


//
// /opt/nec/ve/bin/ncc -shared -fpic -pthread -o libvestackargs.so libvestackargs.c
//
#include <stdio.h>

void ftest(double *d, char *t, int *i)
{
	int a;
	printf("VE: sp = %p\n", (void *)&a);
	printf("VE: arguments passed: %p, %p, %p\n", (void *)d, (void *)t, (void *)i);
	printf("VE: arguments passed by reference: %f, %s, %d\n", *d, t, *i);
}

long test_many_args(double d0, double d1, double d2, double d3, double d4,
                    double d5, double d6, double d7, double d8, double d9)
{
	double result;
	result = d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9;
	printf("VE: %f + %f + %f + %f + %f + %f + %f + %f + %f + %f = %f\n",
	       d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, result);
	union {
		double d;
		long l;
	} u;
	u.d = result;
	return u.l;
}

int test_32(int i32, unsigned int u32, float f32)
{
	union {
		float f;
		int i;
	} u;
	u.f = f32;
	int i = u.i;
	printf("ve: argument passed: %d, %u, %f (%#08x) \n", i32, u32, f32, i);
	return 0;
}

int test_many_inout(char *in0, int *inout1, float *out2, double d3, double d4,
                    double d5, double d6, double d7, char *out8, int i9)
{
	printf("VE: %s\n", in0);
	printf("VE: %d\n", *inout1);
	++*inout1;
	double s = d3 + d4 + d5 + d6 + d7;
	printf("VE: %f + %f + %f + %f + %f = %f\n", d3, d4, d5, d6, d7, s);
	*out2 = (float)s;
	strncpy(out8, "Hello, 89abcdef", i9 - 1);
	return 1;
}

int test_8(int i8, unsigned int u8)
{
    printf("VE: argument passed: %hhd, %hhu\n", i8, u8);
    return i8;
}

int test_16(int i16, unsigned int u16)
{
    printf("VE: argument passed: %hd, %hu\n", i16, u16);
    return 0;
}

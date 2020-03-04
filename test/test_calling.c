#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define RSA_SIZE 176

#define SETARG(N,V) asm volatile ("or %s" #N ", 0, %0"::"r"(V))

#define ALIGN16(x) ((((x)+15) / 16) * 16)

int64_t testfunc(int64_t a1, int64_t a2, int a3, int a4, int a5, int a6,
                 int a7, int a8, int64_t a9)
{
  uint64_t sp, fp;
  asm volatile ("or %0, 0, %sp": "=r"(sp));
  asm volatile ("or %0, 0, %fp": "=r"(fp));

  printf("testfunc sp=%p fp=%p\n", (void *)sp, (void *)fp);

  printf("in testfunc a1=%ld a2=%ld a3=%d a4=%d a5=%d a6=%d a7=%d a8=%d a9=%ld\n",
         a1, a2, a3, a4, a5, a6, a7, a8, a9);
  return 123;
}

int main()
{
  char stack[RSA_SIZE+8*9+8];
  uint64_t sp, fp;
  size_t stack_size = sizeof(stack);
  uint64_t addr = (uint64_t)&testfunc;
  int64_t args[9] = { -12345, 56789, 0, 0, 0, 0, 0, 0, -12 };
  int nargs = 9;

  asm volatile ("or %0, 0, %sp\n\t"
                "or %1, 0, %fp\n\t": "=r"(sp),"=r"(fp));

  printf("main sp=%p fp=%p\n", (void *)sp, (void *)fp);

  uint64_t stack_top = sp - ALIGN16(stack_size);
  
  printf("stack_size = %d, aligned = %d\n", stack_size, ALIGN16(stack_size));

  
  *((int64_t *)&stack[RSA_SIZE]) = args[0];
  *((int64_t *)&stack[RSA_SIZE+8]) = args[1];
  *((int64_t *)&stack[RSA_SIZE+8*8]) = (int64_t)args[8];

  memcpy((void *)stack_top, stack, stack_size);

  if (nargs == 1) {
    SETARG(0,args[0]);
  } else if (nargs == 2) {
    SETARG(0,args[0]); SETARG(1,args[1]);
  } else if (nargs == 3) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]);
  } else if (nargs == 4) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]); SETARG(3,args[3]);
  } else if (nargs == 5) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]); SETARG(3,args[3]);
    SETARG(4,args[4]);
  } else if (nargs == 6) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]); SETARG(3,args[3]);
    SETARG(4,args[4]); SETARG(5,args[5]);
  } else if (nargs == 7) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]); SETARG(3,args[3]);
    SETARG(4,args[4]); SETARG(5,args[5]); SETARG(6,args[6]);
  } else if (nargs >= 8) {
    SETARG(0,args[0]); SETARG(1,args[1]); SETARG(2,args[2]); SETARG(3,args[3]);
    SETARG(4,args[4]); SETARG(5,args[5]); SETARG(6,args[6]); SETARG(7,args[7]);
  }
  
  asm volatile("or %s12, 0, %0\n\t"          /* target function address */
               "st %fp, 0x0(,%sp)\n\t"       /* save original fp */
               "st %lr, 0x8(,%sp)\n\t"       /* fake prologue */
               "st %got, 0x18(,%sp)\n\t"
               "st %plt, 0x20(,%sp)\n\t"
               "or %fp, 0, %sp\n\t"          /* switch fp to new frame */
               "or %sp, 0, %1"               /* switch sp to new frame */
               :
               :"r"(addr), "r"(stack_top)
               :
               );
  asm volatile("bsic %lr, (,%s12)":::);
  asm volatile("or %sp, 0, %fp\n\t"          /* restore original sp */
               "ld %fp, 0x0(,%sp)"           /* restore original fp */
               :::
               );

  int64_t result;
  asm volatile("or %0, 0, %s0":"=r"(result));

  asm volatile ("or %0, 0, %sp": "=r"(sp));
  asm volatile ("or %0, 0, %fp": "=r"(fp));

  printf("main sp=%p fp=%p\n", (void *)sp, (void *)fp);
  
  printf("in main result=%ld\n", result);
  return 0;
}

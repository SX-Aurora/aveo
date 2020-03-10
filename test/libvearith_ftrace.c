#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

double *a, *b, *c;

void alloc_init(int n)
{
  a = (double *)malloc(n * sizeof(double));
  b = (double *)malloc(n * sizeof(double));
  c = (double *)malloc(n * sizeof(double));
  for (int i=0; i<n; i++) {
    a[i] = (double)(i+1)/(double)n;
    b[i] = (double)(i) + 0.1;
    c[i] = 0.0;
  }
}

void sum(int n)
{
  for(int i=0; i<n; i++) {
    c[i] = a[i] + b[i];
  }
}

void mult_add(int n)
{
  for(int i=0; i<n; i++) {
    c[i] = c[i] * a[i] + b[i];
  }
}

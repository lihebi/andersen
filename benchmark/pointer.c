#include <stdio.h>

int main() {
  int *a;
  int *b;
  int **p;
  int **q;
  int v1;
  int v2;
  int v3;
  a = &v1;
  a = &v2;
  *p = a;
  *q = b;
  b = &v3;
  q = &b;
  b = *p;
  a = b;
  p = q;
  return 0;
}

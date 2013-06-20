#ifndef MAIN_HELPER
#define MAIN_HELPER

#include <stdio.h>
#include <stdlib.h>

#define ELEMENT_TYPE float


void randomInit(ELEMENT_TYPE* data, size_t size)
{
  for (size_t i = 0; i < size; ++i)
    data[i] = rand() / (float)RAND_MAX;
}

void zeroInit(ELEMENT_TYPE* data, size_t size)
{
  for (size_t i = 0; i < size; ++i)
    data[i] = 0.0f;
}

void printMatrix(ELEMENT_TYPE* data, size_t n)
{
  for(size_t i = 0; i < n*n; i++)
  {
    printf(" %.4f", data[i]);
    if(((i + 1) % n) == 0)
      printf(";\n");
  }
  printf("\n");
}

void testResult(
  const ELEMENT_TYPE* A, const ELEMENT_TYPE* B, const ELEMENT_TYPE* C,
  size_t N, size_t i, size_t j)
{
  ELEMENT_TYPE value = 0.0f;
  for (size_t k = 0; k < N; ++k) {
    ELEMENT_TYPE elementA = A[i * N + k];
    ELEMENT_TYPE elementB = B[k * N + j];
    value += elementA * elementB;
  }
  printf("  C[%3u][%3u] is ",(uint)i, (uint)j);
  if (C[i * N + j] == value)
    printf("OK :)");
  else
    printf("NOT OK!!! expected be %f (was %f)", value, C[i * N + j]);
  printf("\n");
}


size_t shrRoundUp( int group_size, int global_size ) 
{
    int r = global_size % group_size;
    if( r == 0 )
        return global_size;
    return global_size + group_size - r;
}

#endif
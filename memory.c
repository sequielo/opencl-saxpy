#include "timing.h"
#include "main-helper.h"
#include "cl-helper.h"


void testSumResult(
  const ELEMENT_TYPE* A, const ELEMENT_TYPE* B, const ELEMENT_TYPE* C, size_t N)
{
  size_t count = 0;
  ELEMENT_TYPE value;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      value = A[i * N + j] + B[i * N + j];
      if (C[i * N + j] != value) {
        printf("  C[%3u][%3u] is NOT OK!!! expected %f (was %f)\n",
          (uint)i, (uint)j, value, C[i * N + j]);
        count++;
      }
    }
  }
  if (count == 0)
    printf("All values are OK :)\n");
}


int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "USO: %s  <dimension>  <vueltas>\n", argv[0]);
    abort();
  }

  const cl_long n = atol(argv[1]);
  const int ntrips = atoi(argv[2]);

  cl_context ctx;
  cl_command_queue queue;

  // create_context_on(CHOOSE_INTERACTIVELY, CHOOSE_INTERACTIVELY, 0, &ctx, &queue, 0);
  // print_device_info_from_queue(queue);

  print_platforms_devices();
  create_context_on(NULL, NULL, 0, &ctx, &queue, 0);

  // --------------------------------------------------------------------------
  // load kernels 
  // --------------------------------------------------------------------------
  char *knl_text = read_file("mat-sum.cl");
  cl_kernel knl = kernel_from_string(ctx, knl_text, "sum", NULL);
  free(knl_text);

  // --------------------------------------------------------------------------
  // allocate and initialize CPU memory
  // --------------------------------------------------------------------------

  cl_long sizeN = n*n;
  ELEMENT_TYPE *a = (ELEMENT_TYPE *) malloc(sizeof(ELEMENT_TYPE) * sizeN);
  if (!a) { perror("alloc x"); abort(); }
  ELEMENT_TYPE *b = (ELEMENT_TYPE *) malloc(sizeof(ELEMENT_TYPE) * sizeN);
  if (!b) { perror("alloc y"); abort(); }
  ELEMENT_TYPE *c = (ELEMENT_TYPE *) malloc(sizeof(ELEMENT_TYPE) * sizeN);
  if (!c) { perror("alloc z"); abort(); }

  srand(2006);
  randomInit(a, sizeN);
  randomInit(b, sizeN);
  zeroInit(c, sizeN);

  // --------------------------------------------------------------------------
  // allocate device memory
  // --------------------------------------------------------------------------
  cl_int status;
  cl_mem buf_a = clCreateBuffer(ctx, CL_MEM_READ_WRITE, 
      sizeof(ELEMENT_TYPE) * sizeN, 0, &status);
  CHECK_CL_ERROR(status, "clCreateBuffer");

  cl_mem buf_b = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
      sizeof(ELEMENT_TYPE) * sizeN, 0, &status);
  CHECK_CL_ERROR(status, "clCreateBuffer");

  cl_mem buf_c = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
      sizeof(ELEMENT_TYPE) * sizeN, 0, &status);
  CHECK_CL_ERROR(status, "clCreateBuffer");

  // --------------------------------------------------------------------------
  // transfer to device
  // --------------------------------------------------------------------------
  CALL_CL_GUARDED(clEnqueueWriteBuffer, (
        queue, buf_a, /*blocking*/ CL_TRUE, /*offset*/ 0,
        sizeN * sizeof(ELEMENT_TYPE), a,
        0, NULL, NULL));

  CALL_CL_GUARDED(clEnqueueWriteBuffer, (
        queue, buf_b, /*blocking*/ CL_TRUE, /*offset*/ 0,
        sizeN * sizeof(ELEMENT_TYPE), b,
        0, NULL, NULL));

  // --------------------------------------------------------------------------
  // run code on device
  // --------------------------------------------------------------------------

  CALL_CL_GUARDED(clFinish, (queue));

  timestamp_type time1, time2;
  get_timestamp(&time1);

  for (int trip = 0; trip < ntrips; ++trip)
  {
    SET_3_KERNEL_ARGS(knl, buf_a, buf_b, buf_c);
    size_t globalWorkSize[] = {n,n};
    
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
        (queue, knl,
         /*dimensions*/ 2, NULL, globalWorkSize, NULL, //gdim, ldim,
         0, NULL, NULL));
  }

  CALL_CL_GUARDED(clFinish, (queue));

  get_timestamp(&time2);
  double elapsed = timestamp_diff_in_seconds(time1,time2);
  printf("%12f s\n", elapsed);
  printStatistics(time1, time2, ntrips, n);

  // --------------------------------------------------------------------------
  // transfer back & check
  // --------------------------------------------------------------------------

  if (! getenv("HIDE_CHECK_RESULTS"))
  {
    CALL_CL_GUARDED(clEnqueueReadBuffer, (
          queue, buf_c, /*blocking*/ CL_TRUE, /*offset*/ 0,
          sizeN * sizeof(ELEMENT_TYPE), c,
          0, NULL, NULL));

    if (n < 30)
    {
      printf("\nMatrix A\n");
      printMatrix(a, n);
      
      printf("\nMatrix B\n");
      printMatrix(b, n);
      
      printf("\nMatrix C = A · B\n");
      printMatrix(c, n);
    }

    testSumResult(a,b,c,n);
  }

  // --------------------------------------------------------------------------
  // clean up
  // --------------------------------------------------------------------------
  CALL_CL_GUARDED(clReleaseMemObject, (buf_a));
  CALL_CL_GUARDED(clReleaseMemObject, (buf_b));
  CALL_CL_GUARDED(clReleaseMemObject, (buf_c));
  CALL_CL_GUARDED(clReleaseKernel, (knl));
  CALL_CL_GUARDED(clReleaseCommandQueue, (queue));
  CALL_CL_GUARDED(clReleaseContext, (ctx));
  free(a);
  free(b);
  free(c);

  return 0;
}
#include "../lib/fftw/fftw-bin/include/fftw3.h"

// int main(int argc, char const *argv[]) {

//   int N;
//   fftwl_complex *in, *out;
//   fftwl_plan my_plan;
//   in = (fftwl_complex *)fftwl_malloc(sizeof(fftwl_complex) * N);
//   out = (fftwl_complex *)fftwl_malloc(sizeof(fftwl_complex) * N);
//   my_plan = fftwl_plan_dft_1d(N, in, out, fftwl_FORWARD, fftwl_ESTIMATE);
//   fftwl_execute(my_plan);
//   fftwl_destroy_plan(my_plan);
//   fftwl_free(in);
//   fftwl_free(out);
//   return 0;
// }

#include <math.h>
#include <stdlib.h>
// #include <fftw3.h>

#define N 16
int main(void) {
  fftwl_complex in[N], out[N], in2[N]; /* double [2] */
  fftwl_plan p, q;
  int i;

  /* prepare a cosine wave */
  for (i = 0; i < N; i++) {
    in[i][0] = cos(3 * 2 * M_PI * i / N);
    in[i][1] = 0;
  }

  /* forward Fourier transform, save the result in 'out' */
  p = fftwl_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftwl_execute(p);
  for (i = 0; i < N; i++)
    printf("freq: %3d %+9.5f %+9.5f I\n", i, out[i][0], out[i][1]);
  fftwl_destroy_plan(p);

  /* backward Fourier transform, save the result in 'in2' */
  printf("\nInverse transform:\n");
  q = fftwl_plan_dft_1d(N, out, in2, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftwl_execute(q);
  /* normalize */
  for (i = 0; i < N; i++) {
    in2[i][0] *= 1. / N;
    in2[i][1] *= 1. / N;
  }
  for (i = 0; i < N; i++)
    printf("recover: %3d %+9.5f %+9.5f I vs. %+9.5f %+9.5f I\n", i, in[i][0],
           in[i][1], in2[i][0], in2[i][1]);
  fftwl_destroy_plan(q);

  fftwl_cleanup();
  return 0;
}
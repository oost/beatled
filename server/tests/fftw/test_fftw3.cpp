#include <fftw3.h>
#include <iostream>

#define NUM_POINTS 20

int main(int argc, const char **argv) {
  std::cout << "Main" << std::endl;
  fftw_complex signal[NUM_POINTS];
  fftw_complex result[NUM_POINTS];

  fftw_plan plan =
      fftw_plan_dft_1d(NUM_POINTS, signal, result, FFTW_FORWARD, FFTW_ESTIMATE);

  // acquire_from_somewhere(signal);
  fftw_execute(plan);
  // do_something_with(result);

  fftw_destroy_plan(plan);
  return 0;
}
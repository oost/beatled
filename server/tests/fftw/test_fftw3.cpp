#include <fftw3.h>
#include <iostream>

#define NUM_POINTS 20

int main(int argc, const char **argv) {
  std::cout << "Main" << std::endl;
  fftwl_complex signal[NUM_POINTS];
  fftwl_complex result[NUM_POINTS];

  fftwl_plan plan = fftwl_plan_dft_1d(NUM_POINTS, signal, result, FFTW_FORWARD,
                                      FFTW_ESTIMATE);

  // acquire_from_somewhere(signal);
  fftwl_execute(plan);
  // do_something_with(result);

  fftwl_destroy_plan(plan);
  return 0;
}
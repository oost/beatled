#include "_kiss_fft_guts.h"
#include "kiss_fftr.h"
#include <iostream>
#include <vector>

class FFT {
public:
  FFT() { std::cout << "FFT constructor" << std::endl; };
  ~FFT() { std::cout << "FFT destructor" << std::endl; };

private:
  kiss_fft_cfg cfg;
  kiss_fft_cpx *fftIn;
  kiss_fft_cpx *fftOut;
  std::vector<std::vector<double>> complexOut;
};

int main(int argc, const char **argv) {
  FFT f;
  return 0;
}
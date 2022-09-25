#ifndef CONFIG_H
#define CONFIG_H

// By default FFTW uses double so simpler...
// typedef float audio_buffer_t;
typedef double audio_buffer_t;

namespace constants {
inline constexpr std::size_t audio_buffer_size = 512;

}

#endif // CONFIG_H
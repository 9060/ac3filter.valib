/*
  Noise buffer
  Class that joins data buffer and noise generator.

  Example 1:
  Create the buffer, allocate memory and fill it with noise
  ----------
  RawNoise noise(size, seed);
  do_test(noise);
  ----------

  Example 2:
  Create the buffer and allocate the memory, but create different test vector
  for each test.
  ----------
  RawNoise noise(size, seed);
  for (int i = 0; i < ntests; i++)
  {
    noise.fill_raw();
    do_test(noise);
  }
  ----------
*/

#ifndef NOISE_BUF
#define NOISE_BUF

#include "buffer.h"
#include "rng.h"

class RawNoise : public Rawdata
{
public:
  RNG rng;

  RawNoise()
  {}

  RawNoise(size_t size, int seed):
  rng(seed), Rawdata(size)
  {
    fill_raw();
  }

  inline void fill_raw()
  {
    rng.fill_raw(begin(), size());
  }

  inline void fill_raw(int seed)
  {
    rng.seed(seed);
    rng.fill_raw(begin(), size());
  }

  inline void allocate_noise(size_t size)
  {
    allocate(size);
    fill_raw();
  }

  inline void allocate_noise(size_t size, int seed)
  {
    allocate(size);
    fill_raw(seed);
  }

};

#endif

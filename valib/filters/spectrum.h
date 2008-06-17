/*
  Spectrum analysys filter
*/

#ifndef VALIB_SPECTRUM_H
#define VALIB_SPECTRUM_H

#include "../filter.h"

class Spectrum : public NullFilter
{
protected:
  size_t length;

  double *buf;

  sample_t *data;
  sample_t *spectrum;
  sample_t *win;

  int      *fft_ip;
  sample_t *fft_w;

  size_t pos;
  bool converted;

  bool init();
  void uninit();

  bool on_process();
  void on_reset();

public:
  Spectrum();

  size_t get_length() const;
  bool   set_length(size_t length);
  void   get_spectrum(sample_t *data, double *bin2hz);
};

#endif

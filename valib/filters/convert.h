/*
  PCM format conversions

  Input formats:   PCMxx, Linear
  Ouptupt formats: PCMxx, Linear
  Format conversions:
    Linear -> PCMxx
    PCMxx  -> Linear
  Default format conversion:
    Linear -> PCM16
  Timing: Preserve original
  Buffering: yes/no
*/

#ifndef CONVERT_H
#define CONVERT_H

#include "filter.h"

///////////////////////////////////////////////////////////////////////////////
// Converter class
///////////////////////////////////////////////////////////////////////////////

class Converter : public NullFilter
{
protected:
  // conversion function pointer
  typedef void (Converter::*convert_t)();
  convert_t convert;

  // format
  int  format;             // format to convert to
  int  order[NCHANNELS];   // channel order to convert to when converting from linear format
                           // channel order to convert from when converting to linear format

  // converted samples buffer
  DataBuf buf;             // buffer for converted data
  size_t nsamples;         // buffer size in samples

  // output data pointers
  uint8_t  *out_rawdata;   // buffer pointer for pcm data
  samples_t out_samples;   // buffer pointers for linear data
  size_t    out_size;      // buffer size in bytes/samples for pcm/linear data

  // part of sample from previous call
  uint8_t   part_buf[24];  // partial sample left from previous call
  size_t    part_size;     // partial sample size in bytes
           

  bool alloc_buffer(); // allocate buffer according to format & number of channels

  inline bool query_format(int format) const;

public:
  Converter(int _nsamples)
  {
    spk = def_spk;
    format = spk.format;
    nsamples = _nsamples;
    memcpy(order, std_order, sizeof(order));

    out_size = 0;
    part_size = 0;

    convert = 0;
  }

  /////////////////////////////////////////////////////////
  // Converter interface

  // buffer size
  size_t get_buffer() const;
  bool   set_buffer(size_t _nsamples);
  // output format
  int  get_format() const;
  bool set_format(int _format);
  // output channel order
  void get_order(int _order[NCHANNELS]) const;
  void set_order(const int _order[NCHANNELS]);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool get_chunk(Chunk *out);

  /////////////////////////////////////////////////////////
  // Conversion functions

  void linear_pcm16_1ch();
  void linear_pcm24_1ch();
  void linear_pcm32_1ch();
  void linear_pcmfloat_1ch();
  void linear_pcm16_le_1ch();
  void linear_pcm24_le_1ch();
  void linear_pcm32_le_1ch();
  void linear_pcmfloat_le_1ch();

  void linear_pcm16_2ch();
  void linear_pcm24_2ch();
  void linear_pcm32_2ch();
  void linear_pcmfloat_2ch();
  void linear_pcm16_le_2ch();
  void linear_pcm24_le_2ch();
  void linear_pcm32_le_2ch();
  void linear_pcmfloat_le_2ch();

  void linear_pcm16_3ch();
  void linear_pcm24_3ch();
  void linear_pcm32_3ch();
  void linear_pcmfloat_3ch();
  void linear_pcm16_le_3ch();
  void linear_pcm24_le_3ch();
  void linear_pcm32_le_3ch();
  void linear_pcmfloat_le_3ch();

  void linear_pcm16_4ch();
  void linear_pcm24_4ch();
  void linear_pcm32_4ch();
  void linear_pcmfloat_4ch();
  void linear_pcm16_le_4ch();
  void linear_pcm24_le_4ch();
  void linear_pcm32_le_4ch();
  void linear_pcmfloat_le_4ch();

  void linear_pcm16_5ch();
  void linear_pcm24_5ch();
  void linear_pcm32_5ch();
  void linear_pcmfloat_5ch();
  void linear_pcm16_le_5ch();
  void linear_pcm24_le_5ch();
  void linear_pcm32_le_5ch();
  void linear_pcmfloat_le_5ch();

  void linear_pcm16_6ch();
  void linear_pcm24_6ch();
  void linear_pcm32_6ch();
  void linear_pcmfloat_6ch();
  void linear_pcm16_le_6ch();
  void linear_pcm24_le_6ch();
  void linear_pcm32_le_6ch();
  void linear_pcmfloat_le_6ch();

  void pcm16_linear_1ch();
  void pcm24_linear_1ch();
  void pcm32_linear_1ch();
  void pcmfloat_linear_1ch();
  void pcm16_le_linear_1ch();
  void pcm24_le_linear_1ch();
  void pcm32_le_linear_1ch();
  void pcmfloat_le_linear_1ch();

  void pcm16_linear_2ch();
  void pcm24_linear_2ch();
  void pcm32_linear_2ch();
  void pcmfloat_linear_2ch();
  void pcm16_le_linear_2ch();
  void pcm24_le_linear_2ch();
  void pcm32_le_linear_2ch();
  void pcmfloat_le_linear_2ch();

  void pcm16_linear_3ch();
  void pcm24_linear_3ch();
  void pcm32_linear_3ch();
  void pcmfloat_linear_3ch();
  void pcm16_le_linear_3ch();
  void pcm24_le_linear_3ch();
  void pcm32_le_linear_3ch();
  void pcmfloat_le_linear_3ch();

  void pcm16_linear_4ch();
  void pcm24_linear_4ch();
  void pcm32_linear_4ch();
  void pcmfloat_linear_4ch();
  void pcm16_le_linear_4ch();
  void pcm24_le_linear_4ch();
  void pcm32_le_linear_4ch();
  void pcmfloat_le_linear_4ch();

  void pcm16_linear_5ch();
  void pcm24_linear_5ch();
  void pcm32_linear_5ch();
  void pcmfloat_linear_5ch();
  void pcm16_le_linear_5ch();
  void pcm24_le_linear_5ch();
  void pcm32_le_linear_5ch();
  void pcmfloat_le_linear_5ch();

  void pcm16_linear_6ch();
  void pcm24_linear_6ch();
  void pcm32_linear_6ch();
  void pcmfloat_linear_6ch();
  void pcm16_le_linear_6ch();
  void pcm24_le_linear_6ch();
  void pcm32_le_linear_6ch();
  void pcmfloat_le_linear_6ch();
};

#endif

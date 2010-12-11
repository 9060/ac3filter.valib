/*
  Filter stress test
  * Call open() with numerous formats
  * Call filter functions in different combinations
  * Feed rawdata filters with noise data:
  ** Demux
  ** Detector
  ** Spdifer/Despdifer
  ** AudioDecoder
*/

#include "filter.h"
#include "source/generator.h"
#include "source/raw_source.h"
#include "../all_filters.h"
#include "fir/param_fir.h"
#include <boost/test/unit_test.hpp>

static const int seed = 457934875;
static const size_t noise_size = 1024*1024; // for noise stress test

enum filter_op
{
  op_open, op_reset, op_flush,
  op_process_once, op_process_fill, op_process_cycle
};

///////////////////////////////////////////////////////////////////////////////
// Filter tester
// Checks post-conditions after filter calls

class FilterTester : public FilterWrapper
{
protected:
  Speakers in_spk, out_spk;

public:
  FilterTester(Filter *f): FilterWrapper(f)
  {
    assert(f);
  }

  virtual bool open(Speakers spk)
  {
    bool result = FilterWrapper::open(spk);
    if (result)
    {
      in_spk = get_input();
      out_spk = get_output();
    }
    else if (is_open())
    {
      if (get_input() != in_spk)
        BOOST_REQUIRE_EQUAL(get_input(), in_spk);

      if (get_output() != out_spk)
        BOOST_REQUIRE_EQUAL(get_output(), out_spk);
    }
    return result;
  }

  virtual bool process(Chunk &in, Chunk &out)
  {
    bool result = FilterWrapper::process(in, out);
    if (result)
    {
      if (new_stream())
        out_spk = get_output();
      else if (out_spk.is_unknown())
        out_spk = get_output();
      else if (get_output() != out_spk)
        BOOST_REQUIRE_EQUAL(get_output(), out_spk);
    }
    if (get_input() != in_spk)
      BOOST_REQUIRE_EQUAL(get_input(), in_spk);
    return result;
  }

  virtual bool flush(Chunk &out)
  {
    bool result = FilterWrapper::flush(out);
    if (result)
    {
      if (new_stream())
        out_spk = get_output();
      else if (out_spk.is_unknown())
        out_spk = get_output();
      else if (get_output() != out_spk)
        BOOST_REQUIRE_EQUAL(get_output(), out_spk);
    }
    if (get_input() != in_spk)
      BOOST_REQUIRE_EQUAL(get_input(), in_spk);
    return result;
  }

  virtual void reset()
  {
    FilterWrapper::reset();
    if (get_input() != in_spk)
      BOOST_REQUIRE_EQUAL(get_input(), in_spk);
    if (get_output() != out_spk && !get_output().is_unknown())
      BOOST_FAIL("reset(): incorrect output format change");
    out_spk = get_output();
  }
};

///////////////////////////////////////////////////////////////////////////////
// open() stress test
// Call open() on the filter with numerous formats

void open_stress_test(Filter *filter)
{
  static const int formats[] = 
  { 
    FORMAT_UNKNOWN, // unspecified format
    FORMAT_RAWDATA,
    FORMAT_LINEAR,
    FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32,
    FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, 
    FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE,
    FORMAT_PES, FORMAT_SPDIF,
    FORMAT_AC3, FORMAT_MPA, FORMAT_DTS,
    FORMAT_LPCM20, FORMAT_LPCM24
  };

  static const int modes[] = 
  {
    0, // unspecified mode
    MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2,
    MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE, MODE_3_2_LFE
  };

  static const int sample_rates[] = 
  {
    0, // unspecified sample rate
    192000, 96000, 48000, 24000, 12000,
    176400, 88200, 44100, 22050, 11025, 
    128000, 64000, 32000, 16000, 8000,
  };

  for (int i_format = 0; i_format < array_size(formats); i_format++)
    for (int i_mode = 0; i_mode < array_size(modes); i_mode++)
      for (int i_sample_rate = 0; i_sample_rate < array_size(sample_rates); i_sample_rate++)
      {
        Speakers spk(formats[i_format], modes[i_mode], sample_rates[i_sample_rate]);
        bool supported = filter->can_open(spk);
        bool open = filter->open(spk);
        if (supported != open)
          BOOST_FAIL("can_open(" << spk.print() << ") lies");
      }
}

///////////////////////////////////////////////////////////////////////////////
// Filter stress test
// Call filter methods in number of combinations

void call_filter(Filter *f, Source *src, Chunk &chunk, filter_op op)
{
  Chunk out;

  switch (op)
  {
  case op_open:
    if (!f->open(f->get_input()))
      BOOST_FAIL("open(" << f->get_input().print() << ") fails");
    return;

  case op_reset:
    f->reset();
    return;

  case op_process_once:
    while (chunk.is_empty())
      if (!src->get_chunk(chunk))
        BOOST_FAIL("Need more source data");
    f->process(chunk, out);
    return;


  case op_process_fill:
    do {
      while (chunk.is_empty())
        if (!src->get_chunk(chunk))
          BOOST_FAIL("Need more source data");
    } while (!f->process(chunk, out));
    return;

  case op_process_cycle:
    do {
      while (chunk.is_empty())
        if (!src->get_chunk(chunk))
          BOOST_FAIL("Need more source data");
    } while (!f->process(chunk, out));

    while (f->process(chunk, out))
    {} // do nothing

    return;

  case op_flush:
    while (f->flush(out))
    {} // do nothing

    return;
  }

  assert(false);
}

void filter_stress_test(Filter *f, Source *src, Chunk &chunk, filter_op *ops, size_t nop)
{
  call_filter(f, src, chunk, ops[0]);
  for (size_t i = 2; i < nop; i++)
  {
    call_filter(f, src, chunk, ops[i]);
    call_filter(f, src, chunk, ops[0]);
  }
  if (nop > 1)
    filter_stress_test(f, src, chunk, ops+1, nop-1);
  call_filter(f, src, chunk, ops[0]);
}

void filter_stress_test(Filter *f, Source *src)
{
  filter_op ops[] = {
    op_open, op_reset, op_flush,
    op_process_once, op_process_fill, op_process_cycle
  };

  Chunk chunk;
  filter_stress_test(f, src, chunk, ops, array_size(ops));
}

void filter_stress_test(Speakers spk, Filter *f, const char *filename = 0, size_t data_size = 65536)
{
  static const size_t chunk_size[] = { 1, 2, 1024, 32768 };

  FilterTester t(f);
  for (int i = 0; i < array_size(chunk_size); i++)
  {
    BOOST_REQUIRE(t.open(spk));
    if (filename)
    {
      RAWSource raw(spk, filename, chunk_size[i]);
      BOOST_REQUIRE(raw.is_open());
      filter_stress_test(&t, &raw);
    }
    else
    {
      NoiseGen noise(spk, seed, 15*chunk_size[i] + data_size, chunk_size[i]);
      filter_stress_test(&t, &noise);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Noise stress test
// Feed the filter with noise data

void noise_stress_test(Speakers spk, Filter *filter)
{
  Chunk in, out;
  NoiseGen noise(spk, seed, noise_size);

  BOOST_REQUIRE(filter->open(spk));
  while (noise.get_chunk(in))
    while (filter->process(in, out))
    {} // Do nothing
}

///////////////////////////////////////////////////////////////////////////////

BOOST_TEST_DONT_PRINT_LOG_VALUE(Speakers);

BOOST_AUTO_TEST_SUITE(filter_stress)

BOOST_AUTO_TEST_CASE(agc)
{
  AGC filter(1024);
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(bass_redir)
{
  BassRedir filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_5_1, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(cache)
{
  CacheFilter filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convert_linear2pcm)
{
  Converter filter(1024);
  filter.set_format(FORMAT_PCM16);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convert_pcm2linear)
{
  Converter filter(1024);
  filter.set_format(FORMAT_LINEAR);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convolver)
{
  ParamFIR low_pass(FIR_LOW_PASS, 0.5, 0.0, 0.1, 100, true);
  Convolver filter(&low_pass);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convolver_mch)
{
  ParamFIR low_pass(FIR_LOW_PASS, 0.5, 0.0, 0.1, 100, true);
  ConvolverMch filter;
  // Set only one channel filter (more complex case)
  filter.set_fir(CH_L, &low_pass);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(audio_decoder_ac3)
{
  AudioDecoder filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3");
  noise_stress_test(Speakers(FORMAT_AC3, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(audio_decoder_dts)
{
  AudioDecoder filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.dts");
  noise_stress_test(Speakers(FORMAT_DTS, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(audio_decoder_mpa)
{
  AudioDecoder filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.mp2");
  noise_stress_test(Speakers(FORMAT_MPA, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(decoder_graph)
{
  DecoderGraph filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif");
}

BOOST_AUTO_TEST_CASE(syncer)
{
  Syncer filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(delay)
{
  const float delays[CH_NAMES] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  Delay filter;
  filter.set_units(DELAY_MS);
  filter.set_delays(delays);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(demux)
{
  Demux filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PES, 0, 0), &filter, "a.ac3.03f.pes");
  noise_stress_test(Speakers(FORMAT_PES, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(detector)
{
  Detector filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif");
  noise_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(dvd_graph)
{
  DVDGraph filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PES, 0, 0), &filter, "a.madp.mix.pes");
}

BOOST_AUTO_TEST_CASE(dvd_graph_spdif)
{
  DVDGraph filter;
  filter.set_spdif(true, FORMAT_CLASS_SPDIFABLE, false, true, false);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PES, 0, 0), &filter, "a.madp.mix.pes");
}

BOOST_AUTO_TEST_CASE(levels)
{
  Levels filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(mixer_inplace)
{
  Mixer filter(1024);
  filter.set_output(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(mixer_buffered)
{
  Mixer filter(1024);
  filter.set_output(Speakers(FORMAT_LINEAR, MODE_5_1, 48000));

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(audio_processor)
{
  AudioProcessor filter(1024);
  filter.set_user(Speakers(FORMAT_PCM16, 0, 0));

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter, 0, 131072);
}

BOOST_AUTO_TEST_CASE(resample_up)
{
  Resample filter;
  filter.set_sample_rate(48000);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 44100), &filter);
}

BOOST_AUTO_TEST_CASE(resample_down)
{
  Resample filter;
  filter.set_sample_rate(44100);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(spdifer)
{
  Spdifer filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_AC3, 0, 0), &filter, "a.ac3.03f.ac3");

  noise_stress_test(Speakers(FORMAT_AC3,     0, 0), &filter);
  noise_stress_test(Speakers(FORMAT_MPA,     0, 0), &filter);
  noise_stress_test(Speakers(FORMAT_DTS,     0, 0), &filter);
  noise_stress_test(Speakers(FORMAT_SPDIF,   0, 0), &filter);
  noise_stress_test(Speakers(FORMAT_RAWDATA, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(despdifer)
{
  Despdifer filter;
  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.ac3.03f.spdif");
  noise_stress_test(Speakers(FORMAT_SPDIF, 0, 0), &filter);
}

BOOST_AUTO_TEST_CASE(spectrum)
{
  Spectrum filter;
  filter.set_length(1024);

  open_stress_test(&filter);
  filter_stress_test(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_SUITE_END()

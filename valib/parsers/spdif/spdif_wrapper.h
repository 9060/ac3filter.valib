#ifndef SPDIF_WRAPPER_H
#define SPDIF_WRAPPER_H

#include "../../filter.h"
#include "../../parser.h"
#include "spdifable_header.h"

enum spdif_dts_mode {
  DTS_MODE_AUTO    = 0,
  DTS_MODE_WRAPPED = 1,
  DTS_MODE_PADDED  = 2
};

enum spdif_dts_conv {
  DTS_CONV_NONE  = 0,
  DTS_CONV_16BIT = 1,
  DTS_CONV_14BIT = 2
};

enum spdif_rate_enum {
  SPDIF_RATE_48  = 0x01,
  SPDIF_RATE_44  = 0x02,
  SPDIF_RATE_32  = 0x04,
  SPDIF_RATE_ALL = 0x07
};

enum spdif_passthrough_enum {
  SPDIF_PASS_AC3   = 0x0001,
  SPDIF_PASS_DTS   = 0x0002,
  SPDIF_PASS_MPA   = 0x0004,
  SPDIF_PASS_ALL   = 0x0007,

  HDMI_PASS_EAC3   = 0x0100,
  HDMI_PASS_TRUEHD = 0x0200,
  HDMI_PASS_DTSHD  = 0x0400,
  HDMI_PASS_ALL    = 0x0700,
};

/*
  can_open: returns false if format is not allowed to passthrough or
  sample rate is not allowed for spdif (only in spdif-as-pcm mode).
  If sample rate is not known (=0) it will check the sample rate after
  parsing the first frame and enable passthrough mode if check fails.

  spdif_spk: returns format after spdif conversion. does not check for
  supported format and sample rate.
*/


class SpdifWrapper : public SimpleFilter
{
public:
  SpdifWrapper();

  Speakers spdif_spk(Speakers spk) const;

  int get_passthrough_mask() const;
  void set_passthrough_mask(int passthrough_mask);

  // SPDIF-as-PCM options
  bool get_spdif_as_pcm() const;
  void set_spdif_as_pcm(bool spdif_as_pcm);

  bool get_check_rate() const;
  void set_check_rate(bool check_rate);

  int get_rate_mask() const;
  void set_rate_mask(int rate_mask);

  // DTS options
  int get_dts_mode() const;
  void set_dts_mode(int dts_mode);

  int get_dts_conv() const;
  void set_dts_conv(int dts_conv);

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return out_spk; }

protected:
  // Options
  int passthrough_mask;
  // spdif-as-pcm options
  bool spdif_as_pcm;
  bool check_rate;
  int rate_mask;
  // dts options
  int dts_mode;
  int dts_conv;

  // Processing
  Rawdata buf;         //!< output frame buffer
  SpdifableFrameParser spdifable; //!< All spdifable parsers
  FrameParser *parser; //!< Currently used parser
  FrameInfo finfo;     //!< Current input frame info

  bool passthrough;
  Speakers out_spk;
  bool new_stream_flag;
  int spdif_type;

  bool sync(const FrameInfo &finfo, uint8_t *frame, size_t size);
  size_t wrap(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);

  // AC3
  Speakers spdif_spk_ac3(Speakers spk) const;
  bool sync_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size);
  size_t wrap_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  // EAC3
  Speakers spdif_spk_eac3(Speakers spk) const;
  bool sync_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size);
  size_t wrap_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  // DTS
  Speakers spdif_spk_dts(Speakers spk) const;
  bool sync_dts(const FrameInfo &finfo, uint8_t *frame, size_t size);
  size_t wrap_dts(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  // MPA
  Speakers spdif_spk_mpa(Speakers spk) const;
  bool sync_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size);
  size_t wrap_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
};

#endif

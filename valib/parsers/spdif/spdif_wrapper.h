#ifndef SPDIF_WRAPPER_H
#define SPDIF_WRAPPER_H

#include "../../filter.h"
#include "../../parser.h"
#include "spdifable_header.h"

#define DTS_MODE_AUTO    0
#define DTS_MODE_WRAPPED 1
#define DTS_MODE_PADDED  2

#define DTS_CONV_NONE    0
#define DTS_CONV_16BIT   1
#define DTS_CONV_14BIT   2

class SpdifWrapper : public SimpleFilter
{
public:
  int dts_mode;
  int dts_conv;
  SpdifWrapper(int dts_mode = DTS_MODE_AUTO, int dts_conv = DTS_CONV_NONE);

  FrameInfo frame_info() const { return finfo; }

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
  Rawdata buf; //!< output frame buffer
  SpdifableFrameParser spdifable;
  FrameParser *parser;
  FrameInfo finfo;

  bool passthrough;
  Speakers out_spk;
  bool new_stream_flag;

  int spdif_type;

  bool sync(const FrameInfo &finfo, uint8_t *frame, size_t size);
  bool sync_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size);
  bool sync_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size);
  bool sync_dts(const FrameInfo &finfo, uint8_t *frame, size_t size);
  bool sync_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size);

  size_t wrap(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  size_t wrap_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  size_t wrap_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  size_t wrap_dts(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
  size_t wrap_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest);
};

#endif

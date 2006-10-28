/*
  Use multiple frame parsers at one time
*/

#ifndef MULTI_FRAME_H
#define MULTI_FRAME_H

#include "parser.h"
#include "multi_header.h"

class MultiFrame : public FrameParser
{
protected:

public:
  MultiFrame();
  MultiFrame(FrameParser **parsers, size_t nparsers);

  bool set_parsers(FrameParser **parsers, size_t nparsers);
  void release_parsers();

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual const HeaderParser *header_parser() const { return &multi_header; }

  virtual void reset();
  virtual bool parse_frame(uint8_t *frame, size_t size);

  virtual Speakers  get_spk()      const { return spk;      }
  virtual samples_t get_samples()  const { return samples;  }
  virtual size_t    get_nsamples() const { return nsamples; }

  virtual size_t stream_info(char *buf, size_t size) const;
  virtual size_t frame_info(char *buf, size_t size) const;

protected:
  bool switch_parser(uint8_t *frame, size_t size);

  Speakers  spk;
  samples_t samples;
  int       nsamples;

  FrameParser **parsers;
  size_t nparsers;

  FrameParser *parser;
  const HeaderParser *hparser;

  MultiHeader multi_header;
};

#endif
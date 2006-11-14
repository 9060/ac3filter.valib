#ifndef SPDIF_FRAME_H
#define SPDIF_FRAME_H

#include "parser.h"

class SPDIFFrame : public FrameParser
{
public:
  SPDIFFrame();
  ~SPDIFFrame();

  HeaderInfo header_info() const { return hdr; }

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual const HeaderParser *header_parser() const;

  virtual void reset();
  virtual bool parse_frame(const uint8_t *frame, size_t size);

  virtual Speakers  get_spk()      const { return hdr.spk;      }
  virtual samples_t get_samples()  const { samples_t samples; samples.zero(); return samples; }
  virtual size_t    get_nsamples() const { return hdr.nsamples; }

  virtual const uint8_t *get_rawdata() const { return data;      }
  virtual size_t         get_rawsize() const { return data_size; }

  virtual size_t stream_info(char *buf, size_t size) const;
  virtual size_t frame_info(char *buf, size_t size) const;

protected:
  const uint8_t *data;
  size_t         data_size;
  HeaderInfo     hdr;

  struct spdif_header_s
  {
    uint16_t sync1;   // Pa sync word 1
    uint16_t sync2;   // Pb sync word 2
    uint16_t type;    // Pc data type
    uint16_t len;     // Pd length-code (bits)
  };

};

#endif

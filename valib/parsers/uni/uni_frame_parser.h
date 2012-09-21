#ifndef UNI_FRAME_PARSER_H
#define UNI_FRAME_PARSER_H

#include "../multi_header.h"
#include "../aac/aac_adts_header.h"
#include "../ac3/ac3_header.h"
#include "../dolby/dolby_header.h"
#include "../dts/dts_header.h"
#include "../mpa/mpa_header.h"
#include "../mlp/mlp_header.h"
#include "../spdif/spdif_header.h"

class UniFrameParser : public MultiFrameParser
{
public:
  AC3FrameParser    ac3;
  ADTSFrameParser   adts;
  DolbyFrameParser  dolby;
  DTSFrameParser    dts;
  MPAFrameParser    mpa;
  MlpFrameParser    mlp;
  TruehdFrameParser truehd;
  SPDIFFrameParser  spdif;

  UniFrameParser()
  {
    // mlp & truehd are not included into 'all parsers' because
    // parser becomes too complicated.
    FrameParser *parsers[] = { &adts, &dolby, &dts, &mpa, &spdif};
    set_parsers(parsers, array_size(parsers));
  }

  FrameParser *find_parser(int format)
  {
    switch (format)
    {
      case FORMAT_RAWDATA: return this;
      case FORMAT_AAC_ADTS:return &adts;
      case FORMAT_AC3:     return &dolby;
      case FORMAT_DOLBY:   return &dolby;
      case FORMAT_DTS:     return &dts;
      case FORMAT_EAC3:    return &dolby;
      case FORMAT_MPA:     return &mpa;
      case FORMAT_SPDIF:   return &spdif;
      case FORMAT_MLP:     return &mlp;
      case FORMAT_TRUEHD:  return &truehd;
    }
    return 0;
  }
};

#endif

#ifndef VALIB_SPDIFABLE_HEADER_H
#define VALIB_SPDIFABLE_HEADER_H

#include "../multi_header.h"
#include "../dolby/dolby_header.h"
#include "../dts/dts_header.h"
#include "../mpa/mpa_header.h"
#include "spdif_defs.h"

class SpdifableFrameParser : public MultiFrameParser
{
public:
  DolbyFrameParser dolby;
  DTSFrameParser   dts;
  MPAFrameParser   mpa;

  SpdifableFrameParser()
  {
    FrameParser *parsers[] = { &dolby, &dts, &mpa };
    set_parsers(parsers, array_size(parsers));
  }

  inline const FrameParser *find_parser(int spdif_type) const
  {
    switch (spdif_type)
    {
      // Dolby
      case spdif_type_ac3: return &dolby;
      case spdif_type_eac3: return &dolby;

      // DTS
      case spdif_type_dts_512:
      case spdif_type_dts_1024:
      case spdif_type_dts_2048:
        return &dts;

      // MPA
      case spdif_type_mpeg1_layer1:
      case spdif_type_mpeg1_layer23:
      case spdif_type_mpeg2_lsf_layer1:
      case spdif_type_mpeg2_lsf_layer2:
      case spdif_type_mpeg2_lsf_layer3:
        return &mpa;
    }
    return 0;
  }

  inline FrameParser *find_parser(int spdif_type)
  {
    switch (spdif_type)
    {
      // Dolby
      case spdif_type_ac3: return &dolby;
      case spdif_type_eac3: return &dolby;

      // DTS
      case spdif_type_dts_512:
      case spdif_type_dts_1024:
      case spdif_type_dts_2048:
        return &dts;

      // MPA
      case spdif_type_mpeg1_layer1:
      case spdif_type_mpeg1_layer23:
      case spdif_type_mpeg2_lsf_layer1:
      case spdif_type_mpeg2_lsf_layer2:
      case spdif_type_mpeg2_lsf_layer3:
        return &mpa;
    }
    return 0;
  }
};

#endif

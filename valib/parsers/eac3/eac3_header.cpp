#include "eac3_header.h"

const EAC3Header eac3_header;

static const int srate_tbl[] =
{
  48000, 48000, 48000, 48000, 
  44100, 44100, 44100, 44100, 
  32000, 32000, 32000, 32000, 
  24000, 22050, 16000, 0
};

static const int nsamples_tbl[] =
{
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  1536, 1536, 1536, 1536
};

static const int mask_tbl[] = 
{
  MODE_2_0,
  MODE_2_0 | CH_MASK_LFE,
  MODE_1_0, 
  MODE_1_0 | CH_MASK_LFE, 
  MODE_2_0,
  MODE_2_0 | CH_MASK_LFE,
  MODE_3_0,
  MODE_3_0 | CH_MASK_LFE,
  MODE_2_1,
  MODE_2_1 | CH_MASK_LFE,
  MODE_3_1,
  MODE_3_1 | CH_MASK_LFE,
  MODE_2_2,
  MODE_2_2 | CH_MASK_LFE,
  MODE_3_2,
  MODE_3_2 | CH_MASK_LFE,
};

bool
EAC3Header::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  int bsid, frame_size, sample_rate, nsamples, mode;

  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit big endian stream sync
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
  {
    if ((hdr[4] >> 4) == 0xf)
      return false;

    bsid = hdr[5] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    if (!hinfo)
      return true;

    frame_size = (((hdr[2] & 7) << 8) | hdr[3]) * 2 + 2;
    sample_rate = srate_tbl[hdr[4] >> 4];
    nsamples = nsamples_tbl[hdr[4] >> 4];
    mode = mask_tbl[hdr[4] & 0xf];
    hinfo->bs_type = BITSTREAM_8;
  }
  /////////////////////////////////////////////////////////
  // 16 bit low endian stream sync
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
  {
    if ((hdr[5] >> 4) == 0xf)
      return false;

    bsid = hdr[4] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    if (!hinfo)
      return true;

    frame_size = (((hdr[3] & 7) << 8) | hdr[2]) * 2 + 2;
    sample_rate = srate_tbl[hdr[5] >> 4];
    nsamples = nsamples_tbl[hdr[5] >> 4];
    mode = mask_tbl[hdr[5] & 0xf];
    hinfo->bs_type = BITSTREAM_16LE;
  }
  else
    return false;

  hinfo->spk = Speakers(FORMAT_EAC3, mode, sample_rate);
  hinfo->frame_size = frame_size;
  hinfo->scan_size = 0; // do not scan
  hinfo->nsamples = nsamples;
  hinfo->spdif_type = 1; // SPDIF Pc burst-info (data type = AC3) 
  return true;
}

bool
EAC3Header::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit big endian
  if ((hdr1[0] == 0x0b) && (hdr1[1] == 0x77))
  {
    if ((hdr1[4] >> 4) == 0xf)
      return false;

    int bsid = hdr1[5] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    return hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] &&
           hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] &&
           hdr1[4] == hdr2[4] && (hdr1[5] & 0xf8) == (hdr2[5] & 0xf8);
  }
  /////////////////////////////////////////////////////////
  // 16 bit low endian
  else if ((hdr1[1] == 0x0b) && (hdr1[0] == 0x77))
  {
    if ((hdr1[5] >> 4) == 0xf)
      return false;

    int bsid = hdr1[4] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    return hdr1[1] == hdr2[1] && hdr1[0] == hdr2[0] &&
           hdr1[3] == hdr2[3] && hdr1[2] == hdr2[2] &&
           hdr1[5] == hdr2[5] && (hdr1[4] & 0xf8) == (hdr2[4] & 0xf8);
  }
  return false;
}

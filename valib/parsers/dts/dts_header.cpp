#include "dts_header.h"
#include "bitstream.h"

const DTSHeader dts_header;

static const int dts_sample_rates[] =
{
  0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
  12000, 24000, 48000, 96000, 192000
};

static const int amode2mask_tbl[] = 
{
  MODE_MONO,   MODE_STEREO,  MODE_STEREO,  MODE_STEREO,  MODE_STEREO,
  MODE_3_0,    MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2
};

static const int amode2rel_tbl[] = 
{
  NO_RELATION,   NO_RELATION,  NO_RELATION,  RELATION_SUMDIFF, RELATION_DOLBY,
  NO_RELATION,   NO_RELATION,  NO_RELATION,  NO_RELATION,      NO_RELATION,
};


bool
DTSHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  int bs_type;

  // 16 bits big endian bitstream
  if      (hdr[0] == 0x7f && hdr[1] == 0xfe &&
           hdr[2] == 0x80 && hdr[3] == 0x01)
    bs_type = BITSTREAM_16BE;

  // 16 bits low endian bitstream
  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
           hdr[2] == 0x01 && hdr[3] == 0x80)
    bs_type = BITSTREAM_16LE;

  // 14 bits big endian bitstream
  else if (hdr[0] == 0x1f && hdr[1] == 0xff &&
           hdr[2] == 0xe8 && hdr[3] == 0x00 &&
           hdr[4] == 0x07 && (hdr[5] & 0xf0) == 0xf0)
    bs_type = BITSTREAM_14BE;

  // 14 bits low endian bitstream
  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
           hdr[2] == 0x00 && hdr[3] == 0xe8 &&
          (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
    bs_type = BITSTREAM_14LE;

  // no sync
  else
    return false;


  ReadBS bs_tmp;
  bs_tmp.set_ptr(hdr, bs_type);
  bs_tmp.get(32);                         // Sync
  bs_tmp.get(6);                          // Frame type(1), Deficit sample count(5)

  int cpf = bs_tmp.get(1);                // CRC present flag

  int nblks = bs_tmp.get(7) + 1;          // Number of PCM sample blocks
  if (nblks < 6) return false;            // constraint

  size_t frame_size = bs_tmp.get(14) + 1; // Primary frame byte size
  if (frame_size < 96) return false;      // constraint

  int amode = bs_tmp.get(6);              // Audio channel arrangement
  if (amode > 0xc) return false;          // we don't work with more than 6 channels

  int sfreq = bs_tmp.get(4);              // Core audio sampling frequency
  if (!dts_sample_rates[sfreq])           // constraint
    return false; 

  bs_tmp.get(15);                         // Transmission bit rate(5), and other flags....

  int lff = bs_tmp.get(2);                // Low frequency effects flag
  if (lff == 3) return false;             // constraint

  if (!hinfo)
    return true;

  int sample_rate = dts_sample_rates[sfreq];
  int mask = amode2mask_tbl[amode];
  int relation = amode2rel_tbl[amode];
  if (lff) mask |= CH_MASK_LFE;

  hinfo->spk = Speakers(FORMAT_DTS, mask, sample_rate, 1.0, relation);
  hinfo->frame_size = 0; // do not rely on the frame size specified at header!!!
  hinfo->bs_type = bs_type;
  hinfo->nsamples = nblks * 32;
  hinfo->scan_size = 16384; // always scan up to maximum DTS frame size

  switch (hinfo->nsamples)
  {
    case 512:  hinfo->spdif_type = 11; break;
    case 1024: hinfo->spdif_type = 12; break;
    case 2048: hinfo->spdif_type = 13; break;
    default:   hinfo->spdif_type = 0;  break; // cannot do SPDIF passthrough
  }

  return true;
}


bool
DTSHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  // Compare only:
  // * syncword
  // * AMODE
  // * SFREQ
  // * LFF (do not compare interpolation type)

  // 16 bits big endian bitstream
  if      (hdr1[0] == 0x7f && hdr1[1] == 0xfe &&
           hdr1[2] == 0x80 && hdr1[3] == 0x01)
  {
    return 
      hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] && // syncword
      hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] && // syncword

      (hdr1[7] & 0x0f) == (hdr2[7] & 0x0f) &&     // AMODE
      (hdr1[8] & 0xfc) == (hdr2[8] & 0xfc) &&     // AMODE, SFREQ

      ((hdr1[10] & 0x06) != 0) == ((hdr2[10] & 0x06) != 0); // LFF
  }
  // 16 bits low endian bitstream
  else if (hdr1[0] == 0xfe && hdr1[1] == 0x7f &&
           hdr1[2] == 0x01 && hdr1[3] == 0x80)
  {
    return 
      hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] && // syncword
      hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] && // syncword
      (hdr1[6] & 0x0f) == (hdr2[6] & 0x0f) &&     // AMODE
      (hdr1[9] & 0xfc) == (hdr2[9] & 0xfc) &&     // AMODE, SFREQ

      ((hdr1[11] & 0x06) != 0) == ((hdr2[11] & 0x06) != 0); // LFF
  }
  // 14 bits big endian bitstream
  else if (hdr1[0] == 0x1f && hdr1[1] == 0xff &&
           hdr1[2] == 0xe8 && hdr1[3] == 0x00 &&
           (hdr1[4] & 0xfc) == 0x04)
  {
    return 
      hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] && // syncword
      hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] && // syncword
      (hdr1[4] & 0xfc) == (hdr2[4] & 0xfc) &&     // syncword

      (hdr1[8] & 0x03) == (hdr2[8] & 0x03) &&     // AMODE
      hdr1[9] == hdr2[9] &&                       // AMODE, SFREQ

      ((hdr1[12] & 0x18) != 0) == ((hdr2[12] & 0x18) != 0); // LFF
  }
  // 14 bits low endian bitstream
  else if (hdr1[0] == 0xff && hdr1[1] == 0x1f &&
           hdr1[2] == 0x00 && hdr1[3] == 0xe8 &&
           (hdr1[5] & 0xfc) == 0x04)
  {
    return 
      hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] && // syncword
      hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] && // syncword
      (hdr1[5] & 0xfc) == (hdr2[5] & 0xfc) &&     // syncword

      (hdr1[9] & 0x03) == (hdr2[9] & 0x03) &&     // AMODE
      hdr1[8] == hdr2[8] &&                       // AMODE, SFREQ

      ((hdr1[13] & 0x18) != 0) == ((hdr2[13] & 0x18) != 0); // LFF
  }
  // no sync
  else
    return false;
}

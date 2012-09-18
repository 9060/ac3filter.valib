#include <sstream>
#include "../../bitstream.h"
#include "dolby_header.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and tables
///////////////////////////////////////////////////////////////////////////////

// 0x0b77 | 0x770b
const SyncTrie DolbyFrameParser::sync_trie = SyncTrie(0x0b77, 16) | SyncTrie(0x770b, 16);

static const size_t header_size = 12;

static const int ac3_srate_tbl[4] =
{
  48000, 44100, 32000, 0
};

static const size_t ac3_frame_size_tbl[3][64] =
{
  {
     64,  64,  80,  80,  96,  96, 112, 112,
    128, 128, 160, 160, 192, 192, 224, 224,
    256, 256, 320, 320, 384, 384, 448, 448,
    512, 512, 640, 640, 768, 768, 896, 896,
   1024,1024,1152,1152,1280,1280,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
  }, 
  {
     69,  70,  87,  88, 104, 105, 121, 122,
    139, 140, 174, 175, 208, 209, 243, 244,
    278, 279, 348, 349, 417, 418, 487, 488,
    557, 558, 696, 697, 835, 836, 975, 976,
   1114,1115,1253,1254,1393,1394,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
  }, 
  {
     96,  96, 120, 120, 144, 144, 168, 168,
    192, 192, 240, 240, 288, 288, 336, 336,
    384, 384, 480, 480, 576, 576, 672, 672,
    768, 768, 960, 960,1152,1152,1344,1344,
   1536,1536,1728,1728,1920,1920,   0,   0, 
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
  }, 
};

static const int ac3_lfe_skip_tbl[8] =
{
  0, 0, 2, 2, 2, 4, 2, 4
};

static const int acmod2mask_tbl[8] = 
{
  MODE_2_0,
  MODE_1_0, 
  MODE_2_0,
  MODE_3_0,
  MODE_2_1,
  MODE_3_1,
  MODE_2_2,
  MODE_3_2,
};

static const int eac3_srate_tbl[16] =
{
  48000, 48000, 48000, 48000, 
  44100, 44100, 44100, 44100, 
  32000, 32000, 32000, 32000, 
  24000, 22050, 16000, 0
};

static const int eac3_nsamples_tbl[16] =
{
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  1536, 1536, 1536, 1536
};

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

bool
DolbyFrameParser::SubframeInfo::operator ==(const SubframeInfo &other)
{
  return 
    size == other.size &&
    bs_type == other.bs_type &&
    bsid == other.bsid &&
    independent == other.independent &&
    substream_id == other.substream_id &&
    nsamples == other.nsamples &&
    sample_rate == other.sample_rate &&
    mask == other.mask;
}

bool
DolbyFrameParser::SubframeInfo::operator !=(const SubframeInfo &other)
{
  return !(*this == other);
}

static inline bool is_ac3_bsid(int bsid)
{ return bsid >= 0 && bsid <= 8; }

static inline bool is_eac3_bsid(int bsid)
{ return bsid >= 11 && bsid <= 16; }

static inline bool is_correct_bsid(int bsid)
{ return bsid >= 0 && bsid <= 16 && bsid != 9 && bsid != 10; }

static inline bool chanmap2mask(int chmap)
{}


static bool parse_eac3_subframe_header(const uint8_t *hdr, DolbyFrameParser::SubframeInfo &info)
{
  uint8_t raw_header[header_size];
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
  {
    info.bs_type = BITSTREAM_8;
    memcpy(raw_header, hdr, header_size);
  }
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
  {
    info.bs_type = BITSTREAM_16LE;
    bs_conv_swab16(hdr, header_size, raw_header);
  }

  ReadBS bs(raw_header, 16, header_size * 8);

  int strmtyp = bs.get(2);
  if (strmtyp == 0 || strmtyp == 2)
    info.independent = true;
  else if (strmtyp == 1)
    info.independent = false;
  else
    return false;

  info.substream_id = bs.get(3);
  info.size = bs.get(11) * 2 + 2;

  int fscod_ext = bs.get(4);
  info.sample_rate = eac3_srate_tbl[fscod_ext];
  info.nsamples = eac3_nsamples_tbl[fscod_ext];
  if (!info.sample_rate || !info.nsamples)
    return false;

  int acmod = bs.get(3);
  bool lfeon = bs.get_bool();
  info.mask = acmod2mask_tbl[acmod];
  if (lfeon)
    info.mask |= CH_MASK_LFE;

  info.bsid = bs.get(5);
  if (!is_eac3_bsid(info.bsid))
    return false;

  bs.get(5); // dialnorm
  if (bs.get_bool()) // compre
    bs.get(8); // compr

  if (acmod == 0)
  {
    bs.get(5); // dialnorm2
    if (bs.get_bool()) // compre2
      bs.get(8); // compr2
  }

  bool chanmape = bs.get_bool();
  int chanmap = 0;
  if (chanmape)
    chanmap = bs.get(16);

  return true;
}

static bool parse_ac3_subframe_header(const uint8_t *hdr, DolbyFrameParser::SubframeInfo &info)
{
  info.independent = true;
  info.substream_id = 0;
  info.nsamples = 1536;

  uint8_t raw_header[header_size];
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
  {
    info.bs_type = BITSTREAM_8;
    memcpy(raw_header, hdr, header_size);
  }
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
  {
    info.bs_type = BITSTREAM_16LE;
    bs_conv_swab16(hdr, header_size, raw_header);
  }

  ReadBS bs(raw_header, 32, header_size * 8);

  int fscod = bs.get(2);
  int frmsizecod = bs.get(6);
  if (fscod == 3)
    return false;

  info.sample_rate = ac3_srate_tbl[fscod];
  info.size = ac3_frame_size_tbl[fscod][frmsizecod] * 2;
  
  info.bsid = bs.get(5);
  if (!is_ac3_bsid(info.bsid))
    return false;

  bs.get(3); // bsmod
  int acmod = bs.get(3);
  bs.get(ac3_lfe_skip_tbl[acmod]); // skip cmixlev, surmixlev, dsurmod
  bool lfeon = bs.get_bool();
  info.mask = acmod2mask_tbl[acmod];
  if (lfeon)
    info.mask |= CH_MASK_LFE;

  return true;
}

static bool parse_subframe_header(const uint8_t *hdr, DolbyFrameParser::SubframeInfo &info)
{
  int bsid;
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
    bsid = hdr[5] >> 3;
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
    bsid = hdr[4] >> 3;
  else
    return false;

  if (is_eac3_bsid(bsid))
    return parse_eac3_subframe_header(hdr, info);
  else if (is_ac3_bsid(bsid))
    return parse_ac3_subframe_header(hdr, info);
  else
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// DolbyFrameParser
///////////////////////////////////////////////////////////////////////////////

bool
DolbyFrameParser::can_parse(int format) const
{
  return format == FORMAT_DOLBY || format == FORMAT_EAC3 || format == FORMAT_AC3;
}

bool
DolbyFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  SubframeInfo subframe_info;
  if (!parse_subframe_header(hdr, subframe_info))
    return false;

  if (!subframe_info.independent || subframe_info.substream_id != 0)
    // EAC3 frame may start only at independent stream 0 subframe
    return false;

  if (finfo)
  {
    // Note, that format and channel mask may differ after first_frame() call!
    // Format will change to FORMAT_EAC3 or FORMAT_AC3 and mask may be extended
    // if dependent streams present.
    finfo->spk = Speakers(FORMAT_DOLBY, subframe_info.mask, subframe_info.sample_rate);
    // Frame size is not known because dependent streams may follow
    finfo->frame_size = 0;
    finfo->nsamples = subframe_info.nsamples;
    finfo->bs_type = subframe_info.bs_type;
    finfo->spdif_type = 0;
  }
  return true;
}

bool
DolbyFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  SubframeInfo sfinfo1, sfinfo2;
  if (!parse_subframe_header(hdr1, sfinfo1) || !parse_subframe_header(hdr2, sfinfo2))
    return false;

  return sfinfo1 == sfinfo2;
}

SyncInfo
DolbyFrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  SyncInfo result;
  if (is_ac3_bsid(subframes[0].bsid))
  {
    // For AC3 frame we may use only 16bit sync.
    uint32_t ac3_sync = be2uint16(*(uint16_t *)frame);
    result.sync_trie = SyncTrie(ac3_sync, 16);
  }
  else
  {
    // For EAC3 frame we may use first 45bit for sync (up to bsid).
    uint32_t eac3_sync = be2uint32(*(uint32_t *)frame);
    result.sync_trie = SyncTrie(eac3_sync, 32);
  }
  result.max_frame_size = size;
  result.min_frame_size = size;
  return result;
}

bool
DolbyFrameParser::parse_first_frame(const uint8_t *frame, size_t size, FrameInfo &finfo)
{
  /////////////////////////////////////////////////////////////////////////////
  // Parse subframes

  size_t pos = 0;
  int next_program = 0;
  subframe_count = 0;
  while (pos + ::header_size < size)
  {
    SubframeInfo &sfinfo = subframes[subframe_count];
    if (!parse_subframe_header(frame + pos, sfinfo))
      return false;

    ///////////////////////////////////////////////////////
    // Check program id/substream id

    if (sfinfo.independent)
    {
      if (sfinfo.substream_id != next_program )
        // program ids must be assigned sequentially
        return false;
      next_program++;
    }
    else
    {
      if (subframe_count == 0)
        // first stream is dependent
        return false;

      SubframeInfo &prev_sfinfo = subframes[subframe_count - 1];
      if ((sfinfo.substream_id == 0) != prev_sfinfo.independent ||
           sfinfo.substream_id != 0 && sfinfo.substream_id != prev_sfinfo.substream_id + 1)
        // dependent substream ids must be assigned sequentially
        return false;
    }

    ///////////////////////////////////////////////////////
    // Check subframe constraints

    if (subframe_count)
    {
      if (sfinfo.nsamples != subframes[0].nsamples)
        // All subframes must contain equal number of samples
        return false;

      if (sfinfo.sample_rate != subframes[0].sample_rate)
        // All subframes must have the same sampling rate
        return false;

      if (sfinfo.bs_type != subframes[0].bs_type)
        // All subframes must have the same bitstream type
        return false;
    }

    pos += sfinfo.size;
    subframe_count++;
  }

  if (subframe_count == 0 || pos != size)
    return false;

  /////////////////////////////////////////////////////////////////////////////
  // Update programs

  program_count = -1;
  pos = 0;
  for (int i = 0; i < subframe_count; i++)
  {
    if (subframes[i].independent)
    {
      program_count++;
      programs[program_count].spk = Speakers(FORMAT_EAC3, subframes[i].mask, subframes[i].sample_rate);
      programs[program_count].spk0 = programs[program_count].spk;
      programs[program_count].pos = pos;
      programs[program_count].size = subframes[i].size;
      programs[program_count].first_subframe = i;
      programs[program_count].subframe_count = 1;
    }
    else
    {
      assert(i != 0);
      programs[program_count].spk.mask |= subframes[i].mask;
      programs[program_count].size += subframes[i].size;
      programs[program_count].subframe_count++;
    }
    pos += subframes[i].size;
  }
  program_count++;

  if (subframe_count == 1 && is_ac3_bsid(subframes[0].bsid))
    // Simple AC3 stream
    programs[0].spk.format = programs[0].spk0.format = FORMAT_AC3;

  finfo.spk = programs[0].spk;
  return true;
}

bool
DolbyFrameParser::parse_next_frame(const uint8_t *frame, size_t size, FrameInfo &finfo)
{
  /////////////////////////////////////////////////////////////////////////////
  // Parse subframes

  SubframeInfo sfinfo;
  size_t pos = 0;
  int current_subframe = 0;

  while (pos + ::header_size < size)
  {
    if (!parse_subframe_header(frame + pos, sfinfo))
      return false;

    if (sfinfo != subframes[current_subframe])
      return false;

    pos += sfinfo.size;
    current_subframe++;
  }

  if (current_subframe != subframe_count || pos != size)
    return false;

  finfo.spk = programs[0].spk;
  return true;
}

string
DolbyFrameParser::stream_info() const
{
  if (!in_sync())
    return BasicFrameParser::stream_info();

  std::stringstream result;
  result << BasicFrameParser::stream_info();
  for (int p = 0; p < program_count; p++)
  {
    result << "Program " << p << ": " << programs[p].spk.print() << nl;
    for (int s = programs[p].first_subframe; s < programs[p].first_subframe + programs[p].subframe_count; s++)
    {
      result << "  Substream " << s << ":" << nl;
      result << "    bsid = " << subframes[s].bsid << (is_ac3_bsid(subframes[s].bsid)? " (AC3)": " (EAC3)") << nl;
      result << "    channels = ";
      for (int ch = 0; ch < CH_NAMES; ch++)
        if (subframes[s].mask & CH_MASK(ch))
          result << ch_name_short(ch) << " ";
      result << nl;
    }
  }
  return result.str();
}

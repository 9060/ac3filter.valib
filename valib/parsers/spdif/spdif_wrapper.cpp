#include "../../bitstream.h"
#include "spdif_defs.h"
#include "spdif_wrapper.h"

static const size_t spdif_block_size = 4; // One stereo PCM16 sample = 4 bytes
static const size_t max_spdif_nsamples = 2048;
static const size_t max_spdif_frame_size = max_spdif_nsamples * spdif_block_size;

static const size_t hdmi_block_size2 = 4;  // One 2ch PCM16 sample = 4 bytes.
static const size_t hdmi_block_size4 = 8;  // One 4ch PCM16 sample = 8 bytes.
static const size_t hdmi_block_size6 = 12; // One 6ch PCM16 sample = 12 bytes.
static const size_t hdmi_block_size8 = 16; // One 8ch PCM16 sample = 16 bytes.
static const size_t max_hdmi_nsamples = 2048;
static const size_t max_hdmi_frame_size = max_hdmi_nsamples * hdmi_block_size8;

static const struct {
  int format;
  int spdif_mask;
} spdif_format_tbl[] = {
  { FORMAT_AC3,    SPDIF_PASS_AC3 },
  { FORMAT_MPA,    SPDIF_PASS_MPA },
  { FORMAT_DTS,    SPDIF_PASS_DTS },
  { FORMAT_EAC3,   HDMI_PASS_EAC3 },
  { FORMAT_TRUEHD, HDMI_PASS_TRUEHD },
  { FORMAT_DTS,    HDMI_PASS_DTSHD }
};

static const struct {
  int sample_rate;
  int rate_mask;
} spdif_rate_tbl[] = {
  { 48000, SPDIF_RATE_48 },
  { 44100, SPDIF_RATE_44 },
  { 32000, SPDIF_RATE_32 }
};

struct spdif_header_t
{
  uint16_t zero1;
  uint16_t zero2;
  uint16_t zero3;
  uint16_t zero4;

  uint16_t sync1;   // Pa sync word 1
  uint16_t sync2;   // Pb sync word 2
  uint16_t type;    // Pc data type
  uint16_t len;     // Pd length-code (bits)

  inline void set(uint16_t _type, uint16_t _len_bits)
  {
    zero1 = 0;
    zero2 = 0;
    zero3 = 0;
    zero4 = 0;

    sync1 = 0xf872;
    sync2 = 0x4e1f;
    type  = _type;
    len   = _len_bits;
  }
};

static const size_t header_size = sizeof(spdif_header_t);

static inline bool is_14bit(int bs_type)
{
  return (bs_type == BITSTREAM_14LE) || (bs_type == BITSTREAM_14BE);
}

static FrameParser *find_parser(SpdifableFrameParser &spdifable, Speakers spk)
{
  switch (spk.format)
  {
    case FORMAT_AC3:  return &spdifable.dolby;
    case FORMAT_EAC3: return &spdifable.dolby;
    case FORMAT_DTS:  return &spdifable.dts;
    case FORMAT_MPA:  return &spdifable.mpa;
  }
  return 0;
}

static bool check_format(int passthrough_mask, int format)
{
  for (int i = 0; i < array_size(spdif_format_tbl); i++)
    if ((format == spdif_format_tbl[i].format) &&
        (passthrough_mask & spdif_format_tbl[i].spdif_mask))
      return true;
  return false;
}

static bool check_rate(int rate_mask, int sample_rate)
{
  for (int i = 0; i < array_size(spdif_rate_tbl); i++)
    if ((sample_rate == spdif_rate_tbl[i].sample_rate) &&
        (rate_mask & spdif_rate_tbl[i].rate_mask))
      return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// SpdifWrapper
///////////////////////////////////////////////////////////////////////////////

SpdifWrapper::SpdifWrapper()
: buf(max_hdmi_frame_size)
{
  passthrough_mask = SPDIF_PASS_ALL | HDMI_PASS_ALL;
  spdif_as_pcm = false;
  check_rate = true;
  rate_mask = SPDIF_RATE_ALL;
  dts_mode = DTS_MODE_AUTO;
  dts_conv = DTS_CONV_NONE;
}

int SpdifWrapper::get_passthrough_mask() const
{
  return passthrough_mask;
}

void SpdifWrapper::set_passthrough_mask(int new_passthrough_mask)
{
  passthrough_mask = new_passthrough_mask;
  reset();
}

bool SpdifWrapper::get_spdif_as_pcm() const
{
  return spdif_as_pcm;
}

void SpdifWrapper::set_spdif_as_pcm(bool new_spdif_as_pcm)
{
  spdif_as_pcm = new_spdif_as_pcm;
  reset();
}

bool SpdifWrapper::get_check_rate() const
{
  return check_rate;
}

void SpdifWrapper::set_check_rate(bool new_check_rate)
{
  check_rate = new_check_rate;
  reset();
}

int SpdifWrapper::get_rate_mask() const
{
  return rate_mask;
}

void SpdifWrapper::set_rate_mask(int new_rate_mask)
{
  rate_mask = new_rate_mask;
  reset();
}

int SpdifWrapper::get_dts_mode() const
{
  return dts_mode;
}

void SpdifWrapper::set_dts_mode(int new_dts_mode)
{
  dts_mode = new_dts_mode;
  reset();
}

int SpdifWrapper::get_dts_conv() const
{
  return dts_conv;
}

void SpdifWrapper::set_dts_conv(int new_dts_conv)
{
  dts_conv = new_dts_conv;
  reset();
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
SpdifWrapper::can_open(Speakers spk) const
{
  if (!spdifable.can_parse(spk.format))
    return false;

  if (!check_format(passthrough_mask, spk.format))
    return false;

  // Check sample rate in SPDIF-as-PCM mode
  if (spdif_as_pcm && check_rate && spk.sample_rate &&
    !::check_rate(rate_mask, spk.sample_rate))
    return false;

  return true;
}

bool
SpdifWrapper::init()
{
  reset();
  return true;
}

void 
SpdifWrapper::reset()
{
  out_spk = spk_unknown;
  new_stream_flag = false;

  parser = find_parser(spdifable, spk);
  if (parser)
  {
    passthrough = false;
    parser->reset();
  }
  else
  {
    passthrough = true;
    out_spk = spk;
  }
}

bool
SpdifWrapper::process(Chunk &in, Chunk &out)
{
  out = in;
  in.clear();
  if (passthrough)
  {
    new_stream_flag = false;
    return !out.is_dummy();
  }

  uint8_t *frame = out.rawdata;
  size_t size    = out.size;
  if (size < parser->header_size())
    return false;

  if (parser->in_sync())
  {
    if (parser->next_frame(frame, size))
      new_stream_flag = false;
    else
      parser->reset();
  }

  if (!parser->in_sync())
  {
    if (!parser->first_frame(frame, size))
      return false;

    new_stream_flag = true;
    if (!sync(parser->frame_info(), frame, size))
    {
      // Switch to passthrough mode if sync fails.
      out_spk = spk;
      passthrough = true;
      return true;
    }
  }

  finfo = parser->frame_info();
  size_t spdif_frame_size = wrap(finfo, frame, size, buf);
  if (!spdif_frame_size)
  {
    // Switch to passthrough mode if wrapping fails.
    out_spk = spk;
    passthrough = true;
    new_stream_flag = true;
    return true;
  }

  out.set_rawdata(buf, spdif_frame_size, out.sync, out.time);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

Speakers
SpdifWrapper::spdif_spk(Speakers spk) const
{
  switch (spk.format)
  {
    case FORMAT_AC3:  return spdif_spk_ac3(spk);
    case FORMAT_EAC3: return spdif_spk_eac3(spk);
    case FORMAT_MPA:  return spdif_spk_mpa(spk);
    case FORMAT_DTS:  return spdif_spk_dts(spk);
  }
  return Speakers();
}

bool
SpdifWrapper::sync(const FrameInfo &finfo, uint8_t *frame, size_t size)
{
  out_spk = spdif_spk(finfo.spk);
  switch (finfo.spk.format)
  {
    case FORMAT_AC3:  return sync_ac3(finfo, frame, size);
    case FORMAT_EAC3: return sync_eac3(finfo, frame, size);
    case FORMAT_DTS:  return sync_dts(finfo, frame, size);
    case FORMAT_MPA:  return sync_mpa(finfo, frame, size);
  }
  return false;
}

size_t
SpdifWrapper::wrap(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest)
{
  switch (finfo.spk.format)
  {
    case FORMAT_AC3:  return wrap_ac3(finfo, frame, size, dest);
    case FORMAT_EAC3: return wrap_eac3(finfo, frame, size, dest);
    case FORMAT_DTS:  return wrap_dts(finfo, frame, size, dest);
    case FORMAT_MPA:  return wrap_mpa(finfo, frame, size, dest);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// AC3 (SPDIF)

Speakers
SpdifWrapper::spdif_spk_ac3(Speakers spk) const
{
  if (spdif_as_pcm)
    return Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate);
  return Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate);
}

bool
SpdifWrapper::sync_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size)
{
  spdif_type = spdif_type_ac3;
  return true;
}

size_t
SpdifWrapper::wrap_ac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest)
{
  size_t spdif_frame_size = finfo.nsamples * spdif_block_size;
  if (spdif_frame_size > max_spdif_frame_size || size > spdif_frame_size - header_size)
    return false;

  size_t payload_size = bs_convert(frame, size, finfo.bs_type, dest + header_size, BITSTREAM_16LE);
  assert(payload_size <= max_spdif_frame_size - header_size);
  memset(dest + header_size + payload_size, 0, spdif_frame_size - header_size - payload_size);

  spdif_header_t *header = (spdif_header_t *)dest;
  header->set(spdif_type, (uint16_t)payload_size * 8);
  return spdif_frame_size;
}

///////////////////////////////////////////////////////////////////////////////
// EAC3 (HDMI)

Speakers
SpdifWrapper::spdif_spk_eac3(Speakers spk) const
{
  return Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate * 4);
}

bool
SpdifWrapper::sync_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size)
{
  spdif_type = spdif_type_eac3;
  return true;
}

size_t
SpdifWrapper::wrap_eac3(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest)
{
  size_t hdmi_frame_size = finfo.nsamples * hdmi_block_size2 * 4; // single IEC61937 slot, but 4 times sample rate
  if (hdmi_frame_size > max_hdmi_frame_size || size > hdmi_frame_size - header_size)
    return false;

  size_t payload_size = bs_convert(frame, size, finfo.bs_type, dest + header_size, BITSTREAM_16LE);
  assert(payload_size <= max_hdmi_frame_size - header_size);
  memset(dest + header_size + payload_size, 0, hdmi_frame_size - header_size - payload_size);

  spdif_header_t *header = (spdif_header_t *)dest;
  header->set(spdif_type, (uint16_t)payload_size);
  return hdmi_frame_size;
}

///////////////////////////////////////////////////////////////////////////////
// DTS (SPDIF)

Speakers
SpdifWrapper::spdif_spk_dts(Speakers spk) const
{
  if (spdif_as_pcm)
    return Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate);
  return Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate);
}

bool
SpdifWrapper::sync_dts(const FrameInfo &finfo, uint8_t *frame, size_t size)
{
  int nblks = 0;
  uint16_t *hdr16 = (uint16_t *)frame;

  switch (finfo.bs_type)
  {
    case BITSTREAM_16BE:
      nblks = (be2uint16(hdr16[2]) >> 2)  & 0x7f;
      break;
    case BITSTREAM_16LE:
      nblks = (le2uint16(hdr16[2]) >> 2)  & 0x7f;
      break;
    case BITSTREAM_14BE:
      nblks = (be2uint16(hdr16[2]) << 4)  & 0x70 |
              (be2uint16(hdr16[3]) >> 10) & 0x0f;
      break;
    case BITSTREAM_14LE:
      nblks = (le2uint16(hdr16[2]) << 4)  & 0x70 |
              (le2uint16(hdr16[3]) >> 10) & 0x0f;
      break;
    default:
      return false;
  }

  switch (nblks)
  {
    case 15: spdif_type = spdif_type_dts_512;  break;
    case 31: spdif_type = spdif_type_dts_1024; break;
    case 63: spdif_type = spdif_type_dts_2048; break;
    default: return false;
  }

  return true;
}

size_t
SpdifWrapper::wrap_dts(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest)
{
  size_t spdif_frame_size = finfo.nsamples * spdif_block_size;
  bool use_header = true;
  int spdif_bs = BITSTREAM_16LE;
  // DTS frame may grow if conversion to 14bit stream format is used
  bool frame_grows = (dts_conv == DTS_CONV_14BIT) && !is_14bit(finfo.bs_type);
  // DTS frame may shrink if conversion to 16bit stream format is used
  bool frame_shrinks = (dts_conv == DTS_CONV_16BIT) && is_14bit(finfo.bs_type);

  switch (dts_mode)
  {
  case DTS_MODE_WRAPPED:
    use_header = true;
    if (frame_grows && (size * 8 / 7 <= spdif_frame_size - header_size))
      spdif_bs = BITSTREAM_14LE;
    else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size - header_size))
      spdif_bs = BITSTREAM_16LE;
    else if (size <= spdif_frame_size - header_size)
      spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
    else
      return false;
    break;

  case DTS_MODE_PADDED:
    use_header = false;
    if (frame_grows && (size * 8 / 7 <= spdif_frame_size))
      spdif_bs = BITSTREAM_14LE;
    else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size))
      spdif_bs = BITSTREAM_16LE;
    else if (size <= spdif_frame_size)
      spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
    else
      return false;
    break;

  case DTS_MODE_AUTO:
  default:
    if (frame_grows && (size * 8 / 7 <= spdif_frame_size - header_size))
    {
      use_header = true;
      spdif_bs = BITSTREAM_14LE;
    }
    else if (frame_grows && (size * 8 / 7 <= spdif_frame_size))
    {
      use_header = false;
      spdif_bs = BITSTREAM_14LE;
    }
    else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size - header_size))
    {
      use_header = true;
      spdif_bs = BITSTREAM_16LE;
    }
    else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size))
    {
      use_header = false;
      spdif_bs = BITSTREAM_16LE;
    }
    else if (size <= spdif_frame_size - header_size)
    {
      use_header = true;
      spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
    }
    else if (size <= spdif_frame_size)
    {
      use_header = false;
      spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
    }
    else
      return false;
    break;
  }

  if (use_header)
  {
    size_t payload_size = bs_convert(frame, size, finfo.bs_type, dest + header_size, spdif_bs);
    assert(payload_size < max_spdif_frame_size - header_size);
    memset(dest + header_size + payload_size, 0, spdif_frame_size - header_size - payload_size);

    // We must correct DTS synword when converting to 14bit
    if (spdif_bs == BITSTREAM_14LE)
      dest[header_size + 3] = 0xe8;

    spdif_header_t *header = (spdif_header_t *)dest;
    header->set(spdif_type, (uint16_t)payload_size * 8);
  }
  else
  {
    size_t payload_size = bs_convert(frame, size, finfo.bs_type, dest, spdif_bs);
    assert(payload_size < max_spdif_frame_size);
    memset(dest + payload_size, 0, spdif_frame_size - payload_size);

    // We must correct DTS synword when converting to 14bit
    if (spdif_bs == BITSTREAM_14LE)
      dest[3] = 0xe8;
  }

  return spdif_frame_size;
}

///////////////////////////////////////////////////////////////////////////////
// MPA (SPDIF)

Speakers
SpdifWrapper::spdif_spk_mpa(Speakers spk) const
{
  if (spdif_as_pcm)
    return Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate);
  return Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate);
}

bool
SpdifWrapper::sync_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size)
{
  int version, layer;
  if (finfo.bs_type == BITSTREAM_8)
  {
    version = (frame[1] >> 3) & 0x3;
    layer = (frame[1] >> 1) & 0x3;
  }
  else
  {
    version = (frame[0] >> 3) & 0x3;
    layer = (frame[0] >> 1) & 0x3;
  }

  switch (version)
  {
  case 3: // MPEG1
    if (layer == 3) // Layer1
      spdif_type = spdif_type_mpeg1_layer1;
    else // Layer2 & Layer3
      spdif_type = spdif_type_mpeg1_layer23;
    break;

  case 2: // MPEG2 LSF
    if (layer == 3) // Layer1
      spdif_type = spdif_type_mpeg2_lsf_layer1;
    if (layer == 2) // Layer2
      spdif_type = spdif_type_mpeg2_lsf_layer2;
    else // Layer3
      spdif_type = spdif_type_mpeg2_lsf_layer3;
    break;
  }

  return true;
}

size_t
SpdifWrapper::wrap_mpa(const FrameInfo &finfo, uint8_t *frame, size_t size, uint8_t *dest)
{
  size_t spdif_frame_size = finfo.nsamples * spdif_block_size;
  if (spdif_frame_size > max_spdif_frame_size || size > spdif_frame_size - header_size)
    return false;

  size_t payload_size = bs_convert(frame, size, finfo.bs_type, dest + header_size, BITSTREAM_16LE);
  assert(payload_size <= max_spdif_frame_size - header_size);
  memset(dest + header_size + payload_size, 0, spdif_frame_size - header_size - payload_size);

  spdif_header_t *header = (spdif_header_t *)dest;
  header->set(spdif_type, (uint16_t)payload_size * 8);
  return spdif_frame_size;
}

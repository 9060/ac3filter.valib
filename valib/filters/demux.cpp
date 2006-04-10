#include <string.h>
#include "demux.h"

Demux::Demux()
{
  reset();
}

void 
Demux::process()
{
  out_rawdata = rawdata;
  out_size    = 0;

  while (size)
  {
    if (ps.payload_size)
    {
      size_t len = MIN(size, ps.payload_size);

      // drop all non-audio packets
      if (!ps.is_audio())
      {
        ps.payload_size -= len;
        rawdata += len;
        size    -= len;
        continue;
      }

      // detect stream change
      if ((stream && stream != ps.stream) ||
          (stream && substream && substream != ps.substream))
        break;

      // update stream info
      stream     = ps.stream;
      substream  = ps.substream;
      out_spk    = ps.spk();

      // demux
      memmove(out_rawdata + out_size, rawdata, len);
      ps.payload_size -= len;
      out_size    += len;
      rawdata     += len;
      size        -= len;
    }
    else
    {
      // load PES packet
      uint8_t *end = rawdata + size;
      ps.parse(&rawdata, end);
      size = end - rawdata;
    }
  }
}

void 
Demux::reset()
{
  NullFilter::reset();

  stream    = 0;
  substream = 0;

  out_spk  = spk_unknown;
  out_size = 0;

  ps.reset();
}

bool
Demux::is_ofdd() const
{
  return true;
}

bool 
Demux::query_input(Speakers _spk) const
{
  return _spk.format == FORMAT_PES;
}

bool 
Demux::process(const Chunk *_chunk)
{
  if (!NullFilter::receive_chunk(_chunk))
    return false;

  process();

  if (flushing && !stream)
    // stream was not started so we should not flush it
    flushing = false;

  return true;
}

Speakers 
Demux::get_output() const
{
  return out_spk;
}

bool
Demux::is_empty() const
{
  return !size && !out_size && !flushing;
}

bool
Demux::get_chunk(Chunk *_chunk)
{
  _chunk->set_rawdata
  (
    out_spk, 
    out_rawdata, out_size, 
    sync, time
  );

  // stream change
  if (ps.spk() != out_spk)
  {
    _chunk->eos = true;

    // switch stream
    stream = 0;
    substream = 0;
    out_spk = ps.spk();
  }

  // we must not send end-of-stream if there're data left
  if (!size && flushing)
  {
    _chunk->eos = true;
    flushing = false;

    // go to clear state after flushing
    reset();
  }

  sync = false;
  process();
  return true;
}

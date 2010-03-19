/*
  Filter2
  Base interface for all filters

  /////////////////////////////////////////////////////////
  // Filter usage
  /////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  // Simple case: no format changes

  Filter *filter;
  Chunk2 in, out;
  ...

  if (!filter->open(format))
    error_and_exit();
   
  while (has_more_data())
  {
    in = get_more_data();
    while (!in.is_dummy())
    {
      if (!filter->process(in, out))
        error_and_exit();
      do_something(out);
    }
  }

  while (filter->need_flushing())
  {
    if (!filter->flush(out))
      error_and_exit();
    do_something(out);
  }

  /////////////////////////////////////////////////////////
  // Format change during processing

  Filter *filter;
  ...
  while (has_more_data())
  {
    Chunk2 in = get_more_data();
    Chunk2 out;

    if (new_stream())
    {
      while (filter->need_flushing())
      {
        if (!filter->flush(out))
          error_and_exit();
        do_something(out);
      }
      if (!filter->open(new_format()))
        error_and_exit();
    }

    while (!in.is_dummy())
    {
      if (!filter->process(in, out))
        error_and_exit();
      do_something(out);
    }
  }

  while (filter->need_flushing())
  {
    if (!filter->flush(out))
      error_and_exit();
    do_something(out);
  }

  /////////////////////////////////////////////////////////

  bool can_open(Speakers spk) const
    Check format support. Returns true when filter supports the format given.
    Note that filter may refuse to open the stream even when the stream format
    is supported.

  bool open(const Stream *stream)
    Open the filter. Returns true on success. Filter may process data after
    this call.

  void close()
    Close the filter and deallocate resources.

  bool is_open() const
    Return true when filter is open and can process data.

    Output stream may be set after this call when output format may be
    determined from input format (or when filter does not change the format
    at all).

    But sometimes output format may be determined only after some data received
    (parsers for example). In this case output_format() returns zero until the
    first output chunk.

  bool is_inplace() const
    Returns true when filter modifies data inplace at input buffers. Output
    buffers in this case point inside input buffers.

  /////////////////////////////////////////////////////////
  // Processing

  bool process(Chunk &in, Chunk &out)
    Process data and form one output chunk.

    Filter may process only part of the data. In this case it drops the data
    processed from the input chunk.

    Filter may buffer data. So when it is not enough data at input, filter may
    eat all input and produce no output.

  bool flush(Chunk &out)
    Flush buffered data. We need this only when the filter needs flushing
    (need_flushing() returns true). Note, that filter may produce several
    chunks when flushing, so several flush() calls may be required.

  void reset()
    Reset the filter state, zero all internal buffers and prepare to a new
    stream. Do not deallocate resources, because we're awaiting new data.

  bool new_stream() const
    Filter returns new stream. It may do this for the following reasons:
    * It want the downstream to flush and prepare to receive a new stream
    * It wants to change output format
    Output format returned be get_output() may change when this flag is set.

  bool need_flushing()
    Returns true when filter has buffered data and needs flushing.

  const Stream *get_output()
    Returns output stream. It may return null when output stream is still
    unknown.
*/

#ifndef VALIB_FILTER2_H
#define VALIB_FILTER2_H

#include "filter.h"
#include "buffer.h"

class Chunk2;
class Filter2;
class Stream;
class FilterThunk;


class Chunk2
{
public:
  /////////////////////////////////////////////////////////
  // Data

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

  bool      sync;
  vtime_t   time;

  /////////////////////////////////////////////////////////
  // Utilities

  Chunk2():
    rawdata(0),
    size(0),
    sync(false),
    time(0)
  {}

  Chunk2(Chunk &chunk):
    rawdata(chunk.rawdata),
    samples(chunk.samples),
    size(chunk.size),
    sync(chunk.sync),
    time(chunk.time)
  {}

  Chunk2(bool _sync, vtime_t _time)
  {
    set_empty(_sync, _time);
  }

  Chunk2(samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set_linear(_samples, _size, _sync, _time);
  }

  Chunk2(uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set_rawdata(_rawdata, _size, _sync, _time);
  }

  Chunk2(uint8_t *_rawdata, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set(_rawdata, _samples, _size, _sync, _time);
  }


  inline void set_empty(bool _sync = false, vtime_t _time = 0)
  {
    rawdata = 0;
    samples.zero();
    size = 0;
    sync = _sync;
    time = _time;
  }

  inline void set_linear(samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = 0;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set_rawdata(uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = _rawdata;
    samples.zero();
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set(uint8_t *_rawdata, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = _rawdata;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set_sync(bool _sync, vtime_t _time)
  {
    sync = _sync;
    time = _time;
  }

  inline bool is_dummy() const
  {
    return (size == 0) && !sync;
  }

  inline bool is_empty() const
  { 
    return size == 0; 
  }

  inline void drop_rawdata(size_t _size)
  {
    if (_size > size)
      _size = size;

    rawdata += _size;
    size -= _size;
    sync = false;
  };

  inline void drop_samples(size_t _size)
  {
    if (_size > size)
      _size = size;

    samples += _size;
    size -= _size;
    sync = false;
  };

  Chunk2 &operator =(Chunk &chunk)
  {
    rawdata = chunk.rawdata;
    samples = chunk.samples;
    size = chunk.size;
    sync = chunk.sync;
    time = chunk.time;
  }
};

///////////////////////////////////////////////////////////////////////////////
// Filter2
// Base interface for all filters

class Filter2
{
protected:
  // Prohibit assignment
  Filter2(const Filter2 &);
  Filter2 &operator =(const Filter2 &);
  Filter *thunk;

public:
  Filter2();
  virtual ~Filter2();

  /////////////////////////////////////////////////////////
  // Old filter interface compatibility

  Filter *operator->();
  const Filter *operator->() const;
  operator Filter *();
  operator const Filter *() const;

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;

  // These flags are set after open() and do not change
  virtual bool is_open() const = 0;
  virtual bool is_ofdd() const = 0;
  virtual bool is_inplace() const = 0;
  virtual Speakers get_input() const = 0;

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) = 0;
  virtual bool flush(Chunk2 &out) = 0;
  virtual void reset() = 0;

  // These flags may change during processing
  virtual bool new_stream() const = 0;
  virtual bool need_flushing() const = 0;
  virtual Speakers get_output() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter
// Default implementation for the most of the Filter2 interface.
//
// Only 2 funtions left unimplemented: can_open() and is_inplace() becaluse
// both stongly depends on the filter.


class SimpleFilter : public Filter2
{
protected:
  bool f_open;
  Speakers spk;

public:
  SimpleFilter(): f_open(false)
  {}

  /////////////////////////////////////////////////////////
  // Init/Uninit placeholders

  virtual bool init(Speakers spk) { return true; }
  virtual void uninit() {}

  /////////////////////////////////////////////////////////
  // Open/close the filter
  // Do not override it directly, override init/uninit()
  // placehoilders.

  virtual bool open(Speakers new_spk)
  {
    if (!can_open(new_spk))
      return false;

    if (!init(new_spk))
      return false;

    f_open = true;
    spk = new_spk;
    return true;
  }

  virtual void close()
  {
    uninit();
    f_open = false;
  }

  virtual bool is_open() const
  { return f_open; }

  virtual bool is_ofdd() const
  { return false; }

  virtual Speakers get_input() const
  { return spk; }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool flush(Chunk2 &out)
  { return true; }

  virtual void reset()
  {}

  virtual bool new_stream() const
  { return false; }

  virtual bool need_flushing() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};

///////////////////////////////////////////////////////////////////////////////
// SamplesFilter
// Most of the linear format filters accept any reasonable linear format.
// These class implements can_open() function to accept any fully-specified
// linear format.

class SamplesFilter : public SimpleFilter
{
public:
  SamplesFilter()
  {}

  virtual bool can_open(Speakers spk) const
  {
    return spk.is_linear() && spk.mask != 0 && spk.sample_rate != 0;
  }
};

#endif

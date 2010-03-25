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

  try
  {
    filter->open(format);
    while (has_more_data())
    {
      in = get_more_data();
      while (filter->process(in, out);)
        do_something(out);
    }

    while (filter->flush(out);)
      do_something(out);
  }
  catch (FilterError)
  {
    // Handle filtering errors
  }

  /////////////////////////////////////////////////////////
  // Format change during processing

  Filter *filter;
  ...

  try
  {
    while (has_more_data())
    {
      Chunk2 in = get_more_data();
      Chunk2 out;

      if (new_stream())
      {
        while (filter->flush(out))
          do_something(out);
        filter->open(new_format());
      }

      while (filter->process(in, out))
        filter->process(in, out);
    }

    while (filter->flush(out))
      do_something(out);
  }
  catch (FilterError)
  {
    // Handle filtering errors
  }


  /////////////////////////////////////////////////////////

  bool can_open(Speakers spk) const
    Check format support.
    Return value:
    * true: filter supports the format given. Note that filter may fail to
      to open the stream even when the stream format is supported because
      of resource allocation errors. Also filter may change its mind when
      you change parameters of the filter.
    * false: filter cannot open with the format given.

  bool open(Speakers spk)
    Open the filter and allocate resources.
    Return value:
    * true: success
    * false: fail

  void close()
    Close the filter and deallocate resources.

  bool is_open() const
    Return true when filter is open and can process data.

  /////////////////////////////////////////////////////////
  // Processing

  void reset()
    Reset the filter state, zero all internal buffers and prepare to a new
    stream. Do not deallocate resources, because we're awaiting new data.
    This call should be equivalent to filter.open(filter.get_input()), but we
    do not need to allocate resources once again, just set the internal state
    to initial values.

  bool process(Chunk &in, Chunk &out)
    Process input data and make one output chunk.

    Filter may process only part of the data. In this case it may change input
    chunk's pointers to track the progress. So you may need several process()
    calls with the same input chunk (filter may make several output chunks for
    one input chunk when it is too big).

    Filter may have its own buffer. In this case output chunk points to this
    buffer. When the buffer is not big enough to process the whole input chunk
    at once filter may return several output chunks.

    Filter may process data inplace, i.e. change data at the input buffer and
    return pointers to the input buffer. In some cases you should be warned
    abount this fact. For example imagine that you have 2 filters in the chain
    and the first filter returns its own buffer, and the second is inplace
    filter. You call process on the first and feed inplace filter with the
    output. After this, process() call on the first filter will corrupt data
    at the output of the second filter.

    When filter finds an error and cannot proceed, it must throw FilterError
    exception.

    Return value:
    * true: filter made an output chunk successfully
    * false: filter needs more input data

  bool flush(Chunk &out)
    Flush buffered data. When filter does not require flushing it should just
    return false from this call.

    Note, that filter may produce several chunks when flushing, so several
    flush() calls may be required.

    When filter finds an error and cannot proceed, it must throw FilterError
    exception.

    Return value:
    * true: filter made an output chunk successfully
    * false: filter has done flushing

  bool new_stream() const
    Filter returns a new stream. It may do this for the following reasons:
    * It want the downstream to flush and prepare to receive a new stream
    * It wants to change the output format

  Speakers get_output()
    Returns output format of the filter.

    Generally, output format is known immediately after open() call, so you can
    rely on this and init the next filter in the filter chain. But in some
    cases filter cannot determine the format after open() and have to receive
    some data to say what the output format is. In this case output format must
    be set to FORMAT_UNKNOWN after open(), and to the real format on the first
    output chunk.

    Most filters do not change output format during the processing, but some
    can. To change output format filter must explicitly indicate this with
    help of new_stream() call (it should return true when format changes).
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
    set_empty();
    set_sync(_sync, _time);
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

  inline void set_empty()
  {
    rawdata = 0;
    samples.zero();
    size = 0;
    sync = false;
    time = 0;
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
// FilterError exception

class FilterError
{
public:
  Filter2 *filter;
  const char *name;

  int     error_code;
  const   char *desc;

  FilterError(Filter2 *filter, int error_code, const char *desc);
  virtual ~FilterError();
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

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset() = 0;
  virtual bool process(Chunk2 &in, Chunk2 &out) = 0;
  virtual bool flush(Chunk2 &out) = 0;
  virtual bool new_stream() const = 0;

  // Filter state
  virtual bool     is_open() const = 0;
  virtual bool     is_ofdd() const = 0;
  virtual Speakers get_input() const = 0;
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

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset()
  {}

  virtual bool flush(Chunk2 &out)
  { return false; }

  virtual bool new_stream() const
  { return false; }

  // Filter state

  virtual bool is_open() const
  { return f_open; }

  virtual bool is_ofdd() const
  { return false; }

  virtual Speakers get_input() const
  { return spk; }

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

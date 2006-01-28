/*
  Simple class that acts as file source when filename is specified and as
  noise source otherwise. Useful as test source because not all formats
  require a special file to test.
*/

#ifndef TEST_SOURCE_H
#define TEST_SOURCE_H

#include "source\raw_source.h"
#include "source\noise.h"

class TestSource: public Source
{
protected:
  const char *filename;

  RAWSource   file;   // file source
  Noise       noise;  // noise source
  Source     *source; // current source

public:
  TestSource()
  {
    filename = 0;
    source = 0;
  }

  bool open(Speakers _spk, const char *_filename, size_t _block_size)
  {
    filename = _filename;
    if (_filename)
    {
      source = &file;
      return file.open(_spk, _filename, _block_size);
    }
    else
    {
      source = &noise;
      noise.set_output(_spk, 65536, _block_size);
      noise.set_seed(23545);
      return true;
    }
  }

  bool is_open()
  {
    if (filename)
      return file.is_open();
    else
      return source != 0;
  }

  virtual Speakers get_output() const
  {
    return source? source->get_output(): spk_unknown;
  }
  virtual bool is_empty() const
  {
    return source? source->is_empty(): true;
  }
  virtual bool get_chunk(Chunk *_chunk)
  {
    return source? source->get_chunk(_chunk): false;
  }
};

#endif
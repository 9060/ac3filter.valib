#ifndef SINK_WAV
#define SINK_WAV

#include "filter.h"
#include "auto_file.h"

class WAVSink : public Sink
{
protected:
  AutoFile f;
  Speakers spk;

  uint32_t header_size;  // WAV header size;
  uint32_t data_size;    // data size written to file
  uint8_t *file_format;  // WAVEFORMAT *

public:
  WAVSink();
  WAVSink(const char *file_name);
  ~WAVSink();

  bool open(const char *file_name);
  void close();
  bool is_open() const;

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk); 
};

#endif

/**************************************************************************//**
  \file dolby_header.h
  \brief DolbyFrameParser class.
******************************************************************************/

#ifndef VALIB_DOLBY_HEADER_H
#define VALIB_DOLBY_HEADER_H

#include "../../parser.h"

/**************************************************************************//**
  \class DolbyFrameParser
  \brief Provides DolbyDigial (AC3/EAC3) stream synchronization.

  Can parse following streams:
  - FORMAT_DOLBY
  - FORMAT_AC3
  - FORMAT_EAC3

  EAC3 stream may contain up to 8 independent streams (programs) and up to 8
  dependent streams for each independent.
  
  Independent stream may be decoded without any additional information from
  dependent streams. The set of an independent stream and all dependent
  streams belongs to it referred as 'program'. Dependent streams add and
  replace channels of the independent stream it belongs to allowing more than
  5.1 channels for a program.

  Decoder must be able to decode an independent stream of the first program
  and may skip all other streams.

  Individual frames of a program are multiplexed as follow:

  \verbatim
  +------------+--------------+--------------+--------------+------------+--------------+----
  | IS frame 0 | DS 0 frame 0 | DS 1 frame 0 | DS 2 frame 0 | IS frame 1 | DS 0 frame 1 | ...
  +------------+--------------+--------------+--------------+------------+--------------+----
  |                                                         |
  |<------------------ Program frame 0 -------------------->|<--------- Program frame 1 ----

  IS - independent stream
  DS - dependent stream
  \endverbatim

  The set of all frames of a program will be referred as 'program frame' and
  individual frames as 'subframes' of the program frame. Programs are
  multiplexed similarly:

  \verbatim
  +-------------------+-------------------+-------------------+-------------------+----
  | Program 0 frame 0 | Program 1 frame 0 | Program 2 frame 0 | Program 0 frame 1 | ...
  +-------------------+-------------------+-------------------+-------------------+----
  |                                                           |
  |<---------------------- EAC3 frame 0 --------------------->|<----- EAC3 frame 1 ---
  \endverbatim

  Single EAC3 frame is considered as a set of all program subframes.

  Independent stream may have bsid <= 8 (normal AC3 frame, not EAC3 frame), so
  EAC3 decoder must be able to decode AC3 frames too. Also this means, that AC3
  stream is an particular case of an EAC3 stream with sigle program and without
  dependent streams. So DolbyFrameParser may synchronize on both.

  int DolbyFrameParser::num_programs() const
    Returns number of programs at the EAC3 stream. Valid only when in_sync()
    returns true.

  int DolbyFrameParser::num_subframes() const
    Returns number of subframes (substreams) at the EAC3 stream. Valid only
    when in_sync() returns true.

  const ProgramInfo &DolbyFrameParser::program_info(int program_num) const
    Returns program info. Valid only when in_sync() returns true.

  const SubframeInfo &DolbyFrameParser::subframe_info(int subframe_num) const
    Returns subframe (substream) info. Valid only when in_sync() returns true.

******************************************************************************/

class DolbyFrameParser : public FrameParser
{
public:
  //! Information about a subframe of an EAC3 frame
  struct SubframeInfo
  {
    size_t size;      //!< Subframe size in bytes
    int bs_type;      //!< Bitstream type
    int bsid;         //!< bsid (ac3 version) of the frame
    bool independent; //!< Frame is independently decodable
    int substream_id; //!< Substream id this frame belongs to
    int nsamples;     //!< Number of samples at this frame
    int sample_rate;  //!< Sampling rate
    int mask;         //!< Channels encoded

    bool operator ==(const SubframeInfo &other);
    bool operator !=(const SubframeInfo &other);
  };

  //! Information about EAC3 program
  struct ProgramInfo
  {
    Speakers spk;       //!< Full format (including dependent substreams)
    Speakers spk0;      //!< Format of the independent substream only
    size_t pos;         //!< Position of the first subframe of the program
    size_t size;        //!< Size of all subframes of the program
    int first_subframe; //!< Index of the first subframe of the program
    int subframe_count; //!< Number of subframes per program
  };

  static const SyncTrie sync_trie;

  DolbyFrameParser();

  // Synchronization info
  virtual bool      can_parse(int format) const;
  virtual SyncInfo  sync_info() const;

  // Frame header operations
  virtual size_t    header_size() const;
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

  // Frame operations
  virtual bool      first_frame(const uint8_t *frame, size_t size);
  virtual bool      next_frame(const uint8_t *frame, size_t size);
  virtual void      reset();

  virtual bool      in_sync() const;
  virtual SyncInfo  sync_info2() const;
  virtual FrameInfo frame_info() const;
  virtual string    stream_info() const;

  // EAC3 stream internals
  inline int num_programs() const
  { return program_count; }

  inline int num_subframes() const
  { return subframe_count; }

  inline const ProgramInfo &program_info(int program_num) const
  {
    assert(program_num >= 0 && program_num < program_count);
    return programs[program_num];
  }

  inline const SubframeInfo &subframe_info(int subframe_num) const
  {
    assert(subframe_num >= 0 && subframe_num < subframe_count);
    return subframes[subframe_num];
  }

protected:
  bool sync;
  FrameInfo finfo;
  SyncInfo sinfo;

  int program_count;
  int subframe_count;
  ProgramInfo programs[8];
  SubframeInfo subframes[64];
};

#endif

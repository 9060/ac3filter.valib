/*
  SPDIFParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/spdif/spdif_header.h"
#include "parsers/spdif/spdif_parser.h"
#include "parsers/spdif/spdifable_header.h"
#include "source/file_parser.h"
#include "../../../suite.h"

BOOST_AUTO_TEST_SUITE(spdif_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  SPDIFParser spdif;
}

BOOST_AUTO_TEST_CASE(parse)
{
  FileParser f_spdif;
  f_spdif.open_probe("a.mad.mix.spdif", &spdif_header);
  BOOST_REQUIRE(f_spdif.is_open());

  FileParser f_raw;
  f_raw.open_probe("a.mad.mix.mad", &spdifable_header);
  BOOST_REQUIRE(f_raw.is_open());

  SPDIFParser spdif;
  compare(&f_spdif, &spdif, &f_raw, 0);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open_probe("a.mad.mix.spdif", &spdif_header);
  BOOST_REQUIRE(f.is_open());

  SPDIFParser spdif;
  spdif.open(f.get_output());
  BOOST_CHECK(spdif.is_open());

  Chunk chunk;
  int streams = 0;
  int frames = 0;
  while (f.get_chunk(chunk))
  {
    if (f.new_stream())
    {
      spdif.open(f.get_output());
      BOOST_CHECK(spdif.is_open());
    }
    while (spdif.process(chunk, Chunk()))
    {
      if (spdif.new_stream())
        streams++;
      frames++;
    }
  }

  BOOST_CHECK_EQUAL(streams, 7);
  BOOST_CHECK_EQUAL(frames, 4375);
}

BOOST_AUTO_TEST_SUITE_END()

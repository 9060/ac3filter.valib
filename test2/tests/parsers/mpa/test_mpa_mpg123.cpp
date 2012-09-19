/*
  MPAParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/mpa/mpa_mpg123.h"
#include "parsers/mpa/mpa_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(mpa_mpg123)

BOOST_AUTO_TEST_CASE(constructor)
{
  MPG123Parser mpa;
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain:
  // FileParser -> MPAParser

  FileParser f;
  MPAFrameParser frame_parser;
  f.open_probe("a.mp2.005.mp2", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  MPG123Parser mpa;

  // Reference chain:
  // WAVSource

  WAVSource wav("a.mp2.005.mp2.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  double diff = calc_diff(&f, &mpa, &wav, 0);
  BOOST_CHECK_LE(diff, 1e-6);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  MPAFrameParser frame_parser;
  f.open_probe("a.mp2.mix.mp2", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  MPG123Parser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 3, 1500);
}

BOOST_AUTO_TEST_SUITE_END()

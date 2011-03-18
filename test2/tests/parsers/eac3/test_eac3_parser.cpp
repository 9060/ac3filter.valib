/*
  EAC3Parser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "parsers/eac3/eac3_parser.h"
#include "parsers/eac3/eac3_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(eac3_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  EAC3Parser eac3;
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open_probe("test.eac3.03f.eac3", &eac3_header);
  BOOST_REQUIRE(f.is_open());

  EAC3Parser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 1, 819);
}

BOOST_AUTO_TEST_SUITE_END()

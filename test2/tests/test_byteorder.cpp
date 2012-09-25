/*
  Byteorder functions test.
*/

#include <boost/test/unit_test.hpp>
#include "defs.h"
#include "../suite.h"

static const char     *buf = "\01\02\03\04\05\06\07\08";
static const int32_t  *pi32  = (int32_t *)buf;
static const uint32_t *pui32 = (uint32_t *)buf;
static const int24_t  *pi24  = (int24_t *)buf;
static const int16_t  *pi16  = (int16_t *)buf;
static const uint16_t *pui16 = (uint16_t *)buf;

inline std::ostream &operator <<(std::ostream& o, int24_t i)
{ 
  return o << (int32_t)i;
}

inline bool operator ==(int24_t left, int24_t right)
{
  return (int32_t)left == (int32_t)right;
}

BOOST_AUTO_TEST_SUITE(byteorder)

///////////////////////////////////////////////////////////
// Determine machine endian

BOOST_AUTO_TEST_CASE(endian)
{
  BOOST_CHECK(true); // avoid test with no checks warning.

  if (*pui32 == 0x01020304) 
    BOOST_MESSAGE("Machine uses big endian (Motorola)"); 
  else if (*pui32 == 0x04030201) 
    BOOST_MESSAGE("Machine uses little endian (Intel)"); 
  else
    BOOST_MESSAGE("Unknown endian");
}

BOOST_AUTO_TEST_CASE(swab)
{
  BOOST_CHECK_EQUAL(swab_u32(0x01020304), 0x04030201);
  BOOST_CHECK_EQUAL(swab_s32(0x01020304), 0x04030201);
  BOOST_CHECK_EQUAL(swab_s24(0x010203),   0x030201);
  BOOST_CHECK_EQUAL(swab_u16(0x0102),     0x0201);
  BOOST_CHECK_EQUAL(swab_s16(0x0102),     0x0201);
}

BOOST_AUTO_TEST_CASE(conversion)
{
  /////////////////////////////////////////////////////////
  // Little endian read test (le2intxx functions)

  BOOST_CHECK_EQUAL(le2int32(*pi32),   0x04030201);
  BOOST_CHECK_EQUAL(le2uint32(*pui32), 0x04030201);
  BOOST_CHECK_EQUAL(le2int24(*pi24),   0x030201);
  BOOST_CHECK_EQUAL(le2int16(*pi16),   0x0201);
  BOOST_CHECK_EQUAL(le2uint16(*pui16), 0x0201);

  /////////////////////////////////////////////////////////
  // Big endian read test (be2intxx functions)

  BOOST_CHECK_EQUAL(be2int32(*pi32),   0x01020304);
  BOOST_CHECK_EQUAL(be2uint32(*pui32), 0x01020304);
  BOOST_CHECK_EQUAL(be2int24(*pi24),   0x010203);
  BOOST_CHECK_EQUAL(be2int16(*pi16),   0x0102);
  BOOST_CHECK_EQUAL(be2uint16(*pui16), 0x0102);

  /////////////////////////////////////////////////////////
  // Little endian write test (int2lexx functions)

  BOOST_CHECK_EQUAL(int2le32(0x04030201),  *pi32);
  BOOST_CHECK_EQUAL(uint2le32(0x04030201), *pui32);
  BOOST_CHECK_EQUAL(int2le24(0x030201),    *pi24);
  BOOST_CHECK_EQUAL(int2le16(0x0201),      *pi16);
  BOOST_CHECK_EQUAL(uint2le16(0x0201),     *pui16);

  /////////////////////////////////////////////////////////
  // Big endian write test (int2bexx functions)

  BOOST_CHECK_EQUAL(int2be32(0x01020304),  *pi32);
  BOOST_CHECK_EQUAL(uint2be32(0x01020304), *pui32);
  BOOST_CHECK_EQUAL(int2be24(0x010203),    *pi24);
  BOOST_CHECK_EQUAL(int2be16(0x0102),      *pi16);
  BOOST_CHECK_EQUAL(uint2be16(0x0102),     *pui16);
}

BOOST_AUTO_TEST_CASE(rw)
{
  char test[8];

  /////////////////////////////////////////////////////////
  // Little endian read test

  BOOST_CHECK_EQUAL(read_int32le(buf),  0x04030201);
  BOOST_CHECK_EQUAL(read_uint32le(buf), 0x04030201);
  BOOST_CHECK_EQUAL(read_int24le(buf),  0x030201);
  BOOST_CHECK_EQUAL(read_int16le(buf),  0x0201);
  BOOST_CHECK_EQUAL(read_uint16le(buf), 0x0201);

  /////////////////////////////////////////////////////////
  // Big endian read test

  BOOST_CHECK_EQUAL(read_int32be(buf),  0x01020304);
  BOOST_CHECK_EQUAL(read_uint32be(buf), 0x01020304);
  BOOST_CHECK_EQUAL(read_int24be(buf),  0x010203);
  BOOST_CHECK_EQUAL(read_int16be(buf),  0x0102);
  BOOST_CHECK_EQUAL(read_uint16be(buf), 0x0102);

  /////////////////////////////////////////////////////////
  // Little endian write test

  write_int32le(test, 0x01020304);
  BOOST_CHECK_EQUAL(read_int32le(test), 0x01020304);
  write_uint32le(test, 0x04030201);
  BOOST_CHECK_EQUAL(read_uint32le(test), 0x04030201);
  write_int24le(test, 0x010203);
  BOOST_CHECK_EQUAL(read_int24le(test), 0x010203);
  write_int16le(test, 0x0201);
  BOOST_CHECK_EQUAL(read_int16le(test), 0x00201);
  write_uint16le(test, 0x0102);
  BOOST_CHECK_EQUAL(read_uint16le(test), 0x0102);

  /////////////////////////////////////////////////////////
  // Big endian write test

  write_int32be(test, 0x01020304);
  BOOST_CHECK_EQUAL(read_int32be(test), 0x01020304);
  write_uint32be(test, 0x04030201);
  BOOST_CHECK_EQUAL(read_uint32be(test), 0x04030201);
  write_int24be(test, 0x010203);
  BOOST_CHECK_EQUAL(read_int24be(test), 0x010203);
  write_int16be(test, 0x0403);
  BOOST_CHECK_EQUAL(read_int16be(test), 0x0403);
  write_uint16be(test, 0x0102);
  BOOST_CHECK_EQUAL(read_uint16be(test), 0x0102);
}

BOOST_AUTO_TEST_SUITE_END();

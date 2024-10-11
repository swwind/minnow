#include "wrapping_integers.hh"

#include <cstdlib>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // 将绝对序列号 n 减去 zero_point 的值，然后模 2^32 得到 32 位的相对序列号
  return Wrap32 { static_cast<uint32_t>( n + zero_point.raw_value_ ) };
}

#define ABS( a, b ) ( ( a ) < ( b ) ? ( b - a ) : ( a - b ) )

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t offset = static_cast<uint64_t>( raw_value_ - zero_point.raw_value_ );
  const uint64_t base = checkpoint & 0xffffffff00000000ul;
  const uint64_t c1 = base + offset;
  const uint64_t c2 = c1 < checkpoint ? c1 + 0x100000000ul : c1 - 0x100000000ul;
  return ABS( c1, checkpoint ) < ABS( c2, checkpoint ) ? c1 : c2;
}

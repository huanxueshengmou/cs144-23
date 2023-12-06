#pragma once

#include <cstdint>

/*

Wrap32类型代表一个32位无符号整数，该整数：
从一个任意的"零点"（初始值）开始，并且
当达到2^32 - 1时，回到零。
*/


class Wrap32
{
protected:
  uint32_t raw_value_ {};

public:
  explicit Wrap32( uint32_t raw_value ) : raw_value_( raw_value ) {}

  /* 构造一个 Wrap32，给定一个绝对序列号 n 和零点。*/
  static Wrap32 wrap( uint64_t n, Wrap32 zero_point );

/*

unwrap方法返回一个绝对序列号，该序列号将包装到这个Wrap32，给定零点
以及一个"检查点"：接近期望答案的另一个绝对序列号。
有许多可能的绝对序列号都包装到同一个Wrap32。
unwrap方法应该返回最接近检查点的那个。
*/

  uint64_t unwrap( Wrap32 zero_point, uint64_t checkpoint ) const;

  Wrap32 operator+( uint32_t n ) const { return Wrap32 { raw_value_ + n }; }
  bool operator==( const Wrap32& other ) const { return raw_value_ == other.raw_value_; }
};

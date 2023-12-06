#include "wrapping_integers.hh"
#include <cstdlib>
#include <algorithm>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
   return Wrap32 { zero_point + static_cast<uint32_t>(n) };
}
//zero_point 是uint32_t类型
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
    //求出序列号的循环中的绝对位置
    uint64_t abs_seqno = static_cast<uint64_t>( this->raw_value_ - zero_point.raw_value_ );
    if (abs_seqno>checkpoint)
    {
        return abs_seqno;
    }
    //这里checkpoint>>32求出的是检查点的2的32次倍数，而checkpoint-abs_seqno求出的是从序列号到checkpoint的距离的周期数，相当于把abs_seqno置原点，重新求周期，求一个，从而和检查点处于同一个周期内
    //其实最重要的是吧检查点和序号的周期全部拿走，让其余检查点共处一个相对周期
    uint64_t cycle=((checkpoint-abs_seqno)>>32);
       
    
    uint64_t r=static_cast<uint64_t>((1ull<<32)*(cycle+1)+abs_seqno);
    uint64_t l=static_cast<uint64_t>((1ull<<32)*cycle+abs_seqno);
    if(checkpoint-l>r-checkpoint){
        return r;
    }else{
        return l;
    }
}
/*
但是只需要相对的位置就可以了，因为假设我们的周期只有一个，那么检查点的相对序列和当前相对序列的距离如果大于周期的一半，
说明checkpoint更加接近另一个循环的点，而我们求的是向下取整的cycle，
所以我们后续去加上cycle的时候，是在检查点左侧最近的位置，
而如果我们的序列号相对位置是大于周期一半的话，
由于题目是说要求检查点最近的点，
那么肯定就是在检查点右边最近的，
也就是我们当前的绝对序列号的下一个周期，
所以要进位
由于相对周期要判断的条件太多，还不如选择暴力的最后判断，况且性能还行，就不需要这样做了
*/
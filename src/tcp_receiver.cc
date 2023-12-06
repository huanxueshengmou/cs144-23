#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  /*
  需要使用到的有inser函数
  ：
  Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )

  绝对序号，data，是不是最后一个数据包，output系统
  而output是一个Writer类型，需要外界提供，所以目光在inbound_stream
  message.payload是数据字段
  所以写出来应该是
  reassembler.insert(message.seqno,message.payload,message.FIN,inbound_stream)
  但是应该分别为连接请求和确认和中间段包，和挥手包四种可能
  而reassembler.insert(message.seqno,message.payload,message.FIN,inbound_stream)
  已经包涵了FIN包的处理，以及SYN包的空包处理，也就是说，需要关心的也就first_index这个接口
  当SYN=1时是SYN包，此时没有数据，且设定了初始的绝对位置,此时应该是不需要传递到insert中的
  当SYN=0时是中间段，此时的seqno就是我们的数据index
*/
//此处我不知道SYN如果多发了几次要怎么处理，然后还是不同的情况下，一般来说一个是再开一个线程
//如果此时我们的SYN包接受到，那么我们保存我们已经接受到SYN包,保存一下状态，以及初始的序列号
  if(message.SYN){
    //如果接收到了就保存这个状态
    SYN_received=true;
    //第一次开启的时候会记录一下零点的位置
    zero_point=message.seqno;
    FIN_received=message.FIN;
  }

  abs_seq=message.seqno.unwrap( zero_point, inbound_stream.bytes_pushed() );
  //排除掉我们可能出现的SYN情况，FIN包他本身就可以处理
  if(SYN_received){
    //insert接受的是下一个abs_seq的下标，而abs_seq是依据当前流系统接收的包，拆包后的长度-1是因为只改变index后的数据，以及是否是SYN包是为了包涵当开头时，是0，没有向前的下标了
    reassembler.insert( abs_seq+ message.SYN - 1,message.payload,message.FIN,inbound_stream);
    if(FIN_received){
      //后面可以用作半退出状态去接收数据
      half_close=true;
    }
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  /*
  这里就应该发送SYN包和ACK包，问题是要一起发送还是发送两次？
  看了一下结构体确实是需要发送两次,但是他返回的就一种，那算了，就返回ACK包就行了
  直接通过返回来返回给对方？
  窗户应该设置多大？
  那么我认为inbound_stream似乎没有什么用
  然而是用来获取检查点的
  */
   //由于我们一次连接只需要对SYN响应一次，所以我们响应过一次就设置为false防止误用，这条废除，因为后续连接需要保存syn
  TCPReceiverMessage message_ACK;
  //SYN包：
  uint64_t res = inbound_stream.available_capacity();
  message_ACK.window_size = res > UINT16_MAX ? UINT16_MAX : res;
  if(!SYN_received){
    return message_ACK;
  }

  //这一步网上看到的是用inbound_stream.bytes_pushed当做要装包的数据，我寻思难道不是seqno？
  //这是因为seq插入后，不一定是完全的包，我们会经历合并等等操作，最后返回的应该是我们接受到的有效长度，所以用的是inbound_stream.bytes_pushed()
  //+1是为了当有效长度的一个确认，然后是否是FIN包，我们也需要确认如果是最后一个包，我们分别需要对其seq确认，和FIN包确认，
  //正版的是分开两次确认不知道为什么这两必须是同时合并一步确认
  message_ACK.ackno=temp.wrap( inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed(), zero_point );
   return message_ACK;
}

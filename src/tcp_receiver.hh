#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPReceiver
{
public:
TCPReceiver():SYN_received(false),half_close(false),FIN_received(false),zero_point(0),temp(0),abs_seq(0){}

/*

TCPReceiver接收TCPSenderMessages，将它们的有效载荷插入到正确的流索引的Reassembler中。
*/
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

/* TCPReceiver将TCPReceiverMessages发送回TCPSender。 */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
 private:
  //是不是正在连接中
  bool SYN_received;
  //是不是完成了挥手
  bool half_close;
  //用来标记结束
  bool FIN_received;
  //为ack数做准备
  //初始的绝对长度
  Wrap32 zero_point;
  Wrap32 temp;
  uint64_t abs_seq;
};

#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
using namespace std;
class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_;
  uint64_t window_size=1;
  uint64_t cur_time=0;//这次的总毫秒数
  uint64_t abs_seqno=0;//记录下次的绝对位置
  uint64_t por_abs_ackno=0;//记录上次的绝对位置
  uint64_t abs_ackno=0;//记录下次的绝对位置
  uint8_t retransmissions=0;//记录重传次数
  bool tick_start=false;//是否启动了时钟
  bool has_SYN=false;//连接与否
  bool has_FIN=false;//是否尝试断开
  std::deque<TCPSenderMessage> _outstanding_collections={}; // 记录所有已发送未完成的段
  std::deque<TCPSenderMessage> _ready_collections={};       // 记录所有准备好发送的段
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
   /* 使用给定的默认重传超时和可能的ISN构造TCP发送者 */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  /* 从出站流中推送字节 */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  /* 如果需要发送TCPSenderMessage，则发送（否则返回空的optional） */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
   /* 生成一个空的TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
    /* 接收并响应对端接收者的TCPReceiverMessage */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  /* 自上次调用tick()方法以来，时间已经过去了给定的毫秒数 */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
    /* 用于测试的访问器 */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?// 有多少序列号正在传输中？
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?// 发生了多少次连续的*重*传输？
};

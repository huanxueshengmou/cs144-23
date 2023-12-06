#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <algorithm>
#include <iostream>
using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
  ,RTO_ms_( initial_RTO_ms )
{}
//返回当前在传输中的序列号数量，也就是已经发送但还未收到确认的数据包数量。
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return abs_seqno-por_abs_ackno;
}
//返回连续重传的次数，即同一数据包被重传的次数。
uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions;
}
//该方法尝试发送
optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  //是否没有准备
  if(_ready_collections.empty()){
    return nullopt;
  }
  //如果准备了的话
  TCPSenderMessage maybe_send_Message;
  maybe_send_Message=_ready_collections.front();
  //缓存数据(空包不要缓存)
  if(maybe_send_Message.sequence_length()==0){
    return maybe_send_Message;
  }
  _outstanding_collections.push_back(maybe_send_Message);
  _ready_collections.pop_front();
  //让时钟启动
  tick_start=true;
  return maybe_send_Message;
}
//该方法初定为去放数据
void TCPSender::push( Reader& outbound_stream )
{    // Your code here.
//循环去处理每一个数据，保证其空间可以放入数据即可
//假设看看剩余空间是否仍然有空余（sequence_numbers_in_flight() 的最大值就是 window_size）不会产生uint64_t回滚
  uint64_t available_window_space =max(static_cast<uint64_t>(1), window_size)-sequence_numbers_in_flight();
  //用于测试
  
  while (available_window_space>0)
  {
    // cout <<n<< "available_window_space1:"<<available_window_space<<","<<endl;
    //建立一个默认的消息，范围在0到MAX_PAYLOAD_SIZE
    TCPSenderMessage NewMessages;
    size_t NewMessages_length=(static_cast<size_t>(available_window_space))>=TCPConfig::MAX_PAYLOAD_SIZE?
    TCPConfig::MAX_PAYLOAD_SIZE:(static_cast<size_t>(available_window_space));

    read(outbound_stream,NewMessages_length,NewMessages.payload);
    available_window_space-=NewMessages.payload.size();
    //是否需要SYN包装
    if(!has_SYN){
      has_SYN=true;
      NewMessages.SYN=true;
      NewMessages.seqno=isn_;
    }else{
       NewMessages.seqno = Wrap32::wrap( abs_seqno, isn_ );
    }
    //是否需要FIN包装
    if(!has_FIN&&outbound_stream.is_finished()&&available_window_space>0){
      has_FIN=true;
      NewMessages.FIN=true;
    }
    //是否可以退出（只有当空间不足够以及到结尾以后才会产生空包）
    if(!NewMessages.SYN&&!NewMessages.FIN&&NewMessages.payload.size()==0){
      return;
    }
    //用于测试
    // n++;
    // cout <<n<< "psuh:"<<has_SYN<<","<<endl;
    // cout <<n<< "available_window_space2:"<<available_window_space<<","<<endl;
    // cout <<n<< "window_size:"<<window_size<<","<<endl;
    // cout <<n<< "sequence_numbers_in_flight():"<<sequence_numbers_in_flight()<<","<<endl;
    // cout <<n<< "NewMessages.sequence_length():"<<NewMessages.sequence_length()<<","<<endl;
    // cout <<n<< "abs_seqno:"<<abs_seqno<<","<<endl;
    //均可以放入
    //更新下次的位置
    _ready_collections.push_back(NewMessages);
    abs_seqno += NewMessages.sequence_length();
  }
  
  // //当对方没有空间的时候，我们仍然假设他有一个，让他更新窗口
  // uint16_t window_size=new_win_r-new_win_l; //获取窗口的大小
  // if(window_size==0){
  // _ready_collections.push_back( send_empty_message());
  // return;
  // }
  // //保证已经建立好传输
  // if(has_SYN){
  //   //生成尽可能多的TCPSenderMessage
  //   while(window_size!=0){
  //     TCPSenderMessage NewMessages;
  //     uint16_t NewMessages_length=window_size>=
  //     TCPConfig::MAX_PAYLOAD_SIZE?
  //     TCPConfig::MAX_PAYLOAD_SIZE:window_size;
  //     window_size-=NewMessages_length;
  //     //如果是最后一段，那么我们就标识FIN包
  //     if(outbound_stream.is_finished()){
  //       NewMessages.FIN=true;
  //     }
  //     //只给我看我怎么构建信息啊，我直接引用？
  //     NewMessages.payload{&outbound_stream.peek()};
  //     outbound_stream.pop(NewMessages_length);
  //     NewMessages.seqno=temp.wrap( outbound_stream.bytes_popped() + 1 + outbound_stream.is_finished(), zero_point );
  //     total__ready_collections_length=NewMessages.seqno;
  //     _ready_collections.push_back(NewMessages);
  //   };
  // }else{
  //   //如果没有建立连接就先发出SYN包
  //   TCPSenderMessage Message_SYN;
  //   Message_SYN.SYN=true;
  //   Message_SYN.seqno=isn_;
  //   zero_point=isn.raw_value_;
  //   _ready_collections.push_back(Message_SYN);
  //   has_SYN=true;
  // }
  (void)outbound_stream;
}
//该方法初定为发送空数据
TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  //首先是定义一段空数据
  return TCPSenderMessage{Wrap32::wrap(abs_seqno, isn_ ), false, Buffer(), false};
}
//该方法设置窗口大小，同时删除无用段
void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  //获得窗口左和窗口右(由于窗口大小是uint16MAX所以设置为uint16MAX)
  window_size=msg.window_size;
  //获得abs_ackno
  if(msg.ackno.has_value()){
    abs_ackno=msg.ackno.value().unwrap( isn_, por_abs_ackno );
  }
  //确认新的确认码的有效性
  if(!msg.ackno.has_value()||abs_ackno>abs_seqno||abs_ackno<por_abs_ackno){
    return;
  }
  // 删除已经确认的段
  while (!_outstanding_collections.empty() ) {
    if( _outstanding_collections.front().seqno.unwrap(isn_,abs_ackno) +_outstanding_collections.front().sequence_length() - 1 >= abs_ackno){
      break;
    }
    _outstanding_collections.pop_front();
    // RTO设置
    cur_time = 0;
    RTO_ms_ = initial_RTO_ms_;
    retransmissions = 0;
    // cout << "receive"<<RTO_ms_<<endl;
  }
  //设置时钟关闭
  if(_outstanding_collections.empty()){
    tick_start=false;
  }
  //更新确认
  por_abs_ackno=abs_ackno;
  (void)msg;
}

//该段设置为更新时间
void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  //是否建立启动了时钟,是否建立了连接
  if(!tick_start||!has_SYN){
    // cout <<n<< "是否建立启动了时钟,是否建立了连接"<<"tick_start:"<<tick_start<<","<<"has_SYN:"<<has_SYN<<endl;
    return;
  }
  // //测试temp
  // uint64_t temp=cur_time;
  cur_time+=ms_since_last_tick;
  // cout <<n<< "赋值时的时间"<<"cur_time:"<<temp<<","<<"ms_since_last_tick:"<<cur_time-temp<<endl;
  //是没有超时的话返回
  if(cur_time<RTO_ms_){
    // cout <<n<< "是没有超时的话返回"<<"cur_time:"<<cur_time<<","<<"RTO_ms_:"<<RTO_ms_<<endl;
    return;
  }
  //RTO退避
  if((window_size)>0){
    RTO_ms_*=2;
    //  cout <<n<< "RTO退避"<<"RTO_ms_:"<<RTO_ms_<<","<<"window_size:"<<window_size<<endl;
  }//从新放到我们准备发送的deque
  if(!_outstanding_collections.empty()){
    _ready_collections.push_front(_outstanding_collections.front());
    retransmissions++;
    cur_time=0;
    //  cout <<n<< "从新放到我们准备发送的deque"<<"cur_time:"<<cur_time<<","<<"retransmissions:"<<retransmissions<<endl;
  }
  // cout<<n<<"最后的所有参数一览"<<"ms_since_last_tick"<<ms_since_last_tick<<","
  // <<"has_SYN:"<<has_SYN<<","
  // <<"tick_start:"<<tick_start<<","
  // <<"cur_time:"<<cur_time<<","
  // <<"RTO_ms_:"<<RTO_ms_<<","
  // <<"retransmissions:"<<retransmissions<<endl;
  (void)ms_since_last_tick;
}

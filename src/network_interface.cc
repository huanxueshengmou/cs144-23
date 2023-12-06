#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include <iostream>

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
// ethernet_address: 以太网（ARP协议中称为“硬件”）接口的地址
// ip_address: IP（ARP协议中称为“协议”）接口的地址
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)
// dgram: 要发送的IPv4数据报
// next_hop: 要将数据报发送到的接口的IP地址（通常是路由器或默认网关，但如果与目的地在同一网络上直接连接，
// 也可能是另一台主机）

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
// 注意：Address类型可以通过使用Address::ipv4_numeric()方法转换为uint32_t（原始的32位IP地址）。
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetHeader ethernet_header_;
  EthernetFrame ethernet_Frame_;
  //目的地已知
  auto mac_address= arp_map_.find(next_hop.ipv4_numeric());
  if(mac_address!=arp_map_.end()){
    // cout<<"目的地已知"<<endl;
    ethernet_header_.dst=mac_address->second.macAddress;
    ethernet_header_.src=ethernet_address_;
    ethernet_header_.type=ethernet_header_.TYPE_IPv4;
    ethernet_Frame_.header=ethernet_header_;
    ethernet_Frame_.payload=serialize(dgram);
  }
  //目的地未知,发送ARP帧来取得对应的ip地址与mac映射
  else {
  // cout<<"目的地未知"<<endl;
  //判断是否需要发送（是否找得到记录）
  if(ethernet_frame_ARPMessage_map_.find(next_hop.ipv4_numeric())!=
  ethernet_frame_ARPMessage_map_.end()){
  // cout<<"ARP请求缓冲中"<<endl;
    return;
  }
  // cout<<"执行ARP请求"<<endl;
  ARPMessage new_arp_msg;
  //设置状态码为请求
  new_arp_msg.opcode=ARPMessage::OPCODE_REQUEST;
  new_arp_msg.sender_ethernet_address=ethernet_address_;
  new_arp_msg.sender_ip_address=ip_address_.ipv4_numeric();
  //将发送地址设置成广播地址，但是此处出现了地址错误，说明广播地址是只在以太网帧当中的
  new_arp_msg.target_ip_address=next_hop.ipv4_numeric();

  //添加以太网头部信息
  ethernet_header_.dst=ETHERNET_BROADCAST;
  ethernet_header_.src=ethernet_address_;
  ethernet_header_.type=ethernet_header_.TYPE_ARP;

  //封装以太网帧
  ethernet_Frame_.header=ethernet_header_;
  ethernet_Frame_.payload= std::move( serialize( new_arp_msg ) );
  //放入arp网络优化表，起始行时间是5000ms
  ethernet_frame_ARPMessage_map_[next_hop.ipv4_numeric()]=5000;
  //那么这次是数据也要保存的，因为有可能不知道mac地址，但是仍然存在想要发送消息的愿望
  InternetDatagram_buffer[next_hop.ipv4_numeric()].push_back(dgram);
  }
  //存入到以太网的处理队列之中
  ethernet_frame_buffer_deque.push_back(ethernet_Frame_);
  (void)dgram;
  (void)next_hop;
}

// frame: the incoming Ethernet frame
// frame: 进来的以太网帧
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame ){
  //判断如果mac不是广播或者本地的就直接返回(不是给你的信息)
  if(frame.header.dst!=ethernet_address_&&frame.header.dst!=ETHERNET_BROADCAST){
    return nullopt;
  }
  // cout<<"----------------NetworkInterface::recv_frame-----------------"<<endl;
  
  //传入的是IPV4包
  InternetDatagram internet_datagram;
  if(frame.header.type==EthernetHeader::TYPE_IPv4){
    // cout<<"----------------EthernetHeader::TYPE_IPv4-----------------"<<endl;
    if(parse(internet_datagram,frame.payload)){
      return internet_datagram;
    }
    return nullopt;
  }
  //传入的是ARP帧
  else if(frame.header.type==EthernetHeader::TYPE_ARP){
    ARPMessage arp_msg;
    bool is_effective_arp=parse(arp_msg,frame.payload);
    //需要去判断一下是不是要你发的arp包，即使是发给你的包也有可能是广播的形式，所以也就是要做到mac和ip的双重验证
    if ( arp_msg.target_ip_address != ip_address_.ipv4_numeric() ){
      return nullopt;
    }
    if(is_effective_arp){
      //设置缓存
      DeviceInfo device_info{arp_msg.sender_ethernet_address,30000};
      //如果有值直接覆盖，如果没有创建(更新)
      arp_map_[arp_msg.sender_ip_address]={device_info};
      //应该回复什么？
      //如果是对方发的是ARP包,我们需要把之前未说的种种发送给对方
      if(arp_msg.opcode==ARPMessage::OPCODE_REPLY){
        //把之前没说过的话，都倾述出来
        //转换一下地址
        cout<<"正在发送未处理的数据包的循环前arp_msg.opcode:"<<arp_msg.opcode<<endl;
        Address send_ip_address=Address::from_ipv4_numeric(arp_msg.sender_ip_address);
        for(auto &it : InternetDatagram_buffer[arp_msg.sender_ip_address]){
            send_datagram(it,send_ip_address);
            cout<<"正在发送未处理的数据包的循环中,arp_msg.opcode:"<<arp_msg.opcode<<endl;
        }
        InternetDatagram_buffer[arp_msg.sender_ip_address].clear();
      }
      //如果对方发的是请求，要我们回复arp包
      else{
        //建立arp包
       ARPMessage new_arp_msg;
       //建立包装
       EthernetHeader ethernet_header_;
       EthernetFrame ethernet_Frame_;
       //设置状态码为响应
       new_arp_msg.opcode=ARPMessage::OPCODE_REPLY;
       new_arp_msg.sender_ethernet_address=ethernet_address_;
       new_arp_msg.sender_ip_address=ip_address_.ipv4_numeric();//这里只是处理了ipv4说明有可能出现ipv6需要过滤
       //交换源发送端mac和ip
       new_arp_msg.target_ethernet_address=arp_msg.sender_ethernet_address;
       new_arp_msg.target_ip_address=arp_msg.sender_ip_address;
       ethernet_Frame_.payload=serialize(new_arp_msg);
       //设置类型
       ethernet_header_.type=ethernet_header_.TYPE_ARP;
       //目标地址和源地址交换
       ethernet_header_.dst=frame.header.src;
       ethernet_header_.src=ethernet_address_;
       //替换以太网头
       ethernet_Frame_.header=ethernet_header_;
       //放入准备队列
       ethernet_frame_buffer_deque.push_back(ethernet_Frame_);     
      }
    }
    return nullopt;
  }
  (void)frame;
return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
// ms_since_last_tick: 自上次调用此方法以来的毫秒数
void NetworkInterface::tick( const size_t ms_since_last_tick )
{

  //更新映射，遍历一遍全部更新就是了
  for (auto it = arp_map_.begin(); it != arp_map_.end(); ) {
    it->second.existenceTime=it->second.existenceTime>ms_since_last_tick?
    it->second.existenceTime-ms_since_last_tick:
    0;
    if (it->second.existenceTime==0) { 
        it = arp_map_.erase(it); 
    } else {
        ++it; 
    }
   }
  //更新ARP请求时间
  for (auto it =ethernet_frame_ARPMessage_map_.begin(); it != ethernet_frame_ARPMessage_map_.end(); ) {
    it->second=it->second>ms_since_last_tick?
    it->second-ms_since_last_tick:
    0;
    if (it->second==0) { 
        it = ethernet_frame_ARPMessage_map_.erase(it); 
    } else {
        ++it; 
    }
   }
  (void)ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  /**
   * 按照我们实现的相应部分，这一部分的应该只返回一个消息即可
  */
 //没准备好就发空的
 if(ethernet_frame_buffer_deque.empty()){
  return nullopt;
 }
 //准备好了就发过去
  EthernetFrame frame=ethernet_frame_buffer_deque.front();
  ethernet_frame_buffer_deque.pop_front();
  return frame;
}

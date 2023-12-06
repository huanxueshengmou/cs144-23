#include "router.hh"

#include <iostream>
#include <limits>
#include<iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.

// route_prefix: 要与数据报目的地址匹配的“最多32位”的IPv4地址前缀
// prefix_length: 此路由适用所需的条件，即数据报目的地址的高阶（最重要）位中有多少位需要与路由前缀对应的位匹配？
// next_hop: 下一跳的IP地址。如果网络直接连接到路由器，则为空（在这种情况下，下一跳地址应该是数据报的最终目的地）。
// interface_num: 发送数据报的接口索引。

void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
//什么样的消息我们不要更新路由表？


//设置一下路由信息
  Route_Message route_message;
  route_message.interface_num=interface_num;
  route_message.next_hop=next_hop;
  route_message.prefix_length=prefix_length;
  route_message.route_prefix=route_prefix;
//存储路由信息
  route_map.push_back(route_message);
  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;
}

bool ip_matches_route(uint32_t ip_address, uint32_t route_prefix, uint8_t prefix_length) {
if(prefix_length>32){
  return false;
}
uint32_t subnet_mask = prefix_length == 0 ? 0 : 0xFFFFFFFF << (32 - prefix_length);
uint32_t masked_ip = ip_address & subnet_mask;
uint32_t masked_route_prefix = route_prefix & subnet_mask;
return masked_ip == masked_route_prefix;
}

void Router::route() {
//当前是否有接口传输数据
for(auto &it:interfaces_){
  auto datagram=it.maybe_receive();
  if(!datagram.has_value()){
    continue;
  }
  auto& dgram=datagram.value();
  //是否已经有已经不存在的路由
  if(dgram.header.ttl<=1){
    continue;
  }
  //否者计算校验和，更新ttl
  dgram.header.ttl--;
  dgram.header.compute_checksum();
  //是否找得到匹配一个段?
  //获取ip地址
  uint32_t IP=dgram.header.dst;
  //设置最匹配地址
  Route_Message bestMatchRoute{};
  bool hasBestMatch = false; // 新增变量来跟踪是否找到有效匹配
  size_t i=0;
  while(i<route_map.size()){
    // cout<<"发送之前"<<endl;
    // cout<<"bestMatchRoute.prefix_length:"<<to_string(bestMatchRoute.prefix_length)<<endl;
    // cout<<"route_map[i].prefix_length:"<<to_string(route_map[i].prefix_length)<<endl;
    // cout<<"hasBestMatch:"<<hasBestMatch<<endl;
    // cout<<"IP:"<<IP<<endl;
    

    if(!ip_matches_route(IP,route_map[i].route_prefix,route_map[i].prefix_length)){
      i++;
      continue;
    }
    //当前的是否需要更新，如果没有赋值先赋值，如果已经初始化了就看看是不是需要更新
    if(hasBestMatch&&bestMatchRoute.prefix_length>=route_map[i].prefix_length){
      i++;
      continue;
    }
    
    bestMatchRoute=route_map[i];
    hasBestMatch=true;
    i++;
  }
  //有合适下一跳？
  if(!hasBestMatch){
  continue;
  }
auto &async_NetworkInterface=interface(bestMatchRoute.interface_num);
async_NetworkInterface.send_datagram(dgram, 
            bestMatchRoute.next_hop.value_or(Address::from_ipv4_numeric(IP)));
    // cout<<"发送之后"<<endl;
    // cout<<"bestMatchRoute.prefix_length:"<<to_string(bestMatchRoute.prefix_length)<<endl;
    // cout<<"route_map[i].prefix_length:"<<to_string(route_map[i].prefix_length)<<endl;
    // cout<<"hasBestMatch:"<<hasBestMatch<<endl;
}
}

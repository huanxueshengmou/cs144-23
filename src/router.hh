#pragma once

#include "network_interface.hh"

#include <optional>
#include <queue>

// A wrapper for NetworkInterface that makes the host-side
// interface asynchronous: instead of returning received datagrams
// immediately (from the `recv_frame` method), it stores them for
// later retrieval. Otherwise, behaves identically to the underlying
// implementation of NetworkInterface.

// 对NetworkInterface的包装，使得主机侧的接口变为异步：不是立即返回收到的数据报
//（来自`recv_frame`方法），而是将它们存储起来以供稍后检索。除此之外，
// 其行为与底层实现的NetworkInterface相同。

class AsyncNetworkInterface : public NetworkInterface
{
  std::queue<InternetDatagram> datagrams_in_ {};

public:
  using NetworkInterface::NetworkInterface;

  // Construct from a NetworkInterface
  // 从NetworkInterface构造
  explicit AsyncNetworkInterface( NetworkInterface&& interface ) : NetworkInterface( interface ) {}

  // \brief Receives and Ethernet frame and responds appropriately.

  // - If type is IPv4, pushes to the `datagrams_out` queue for later retrieval by the owner.
  // - If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // - If type is ARP reply, learn a mapping from the "target" fields.
  //
  // \param[in] frame the incoming Ethernet frame

    // \brief 接收Ethernet帧并做出相应的响应。

  // - 如果类型是IPv4，则将其推入`datagrams_out`队列，以供所有者稍后检索。
  // - 如果类型是ARP请求，则学习“发送者”字段中的映射，并发送ARP回复。
  // - 如果类型是ARP回复，则学习“目标”字段中的映射。
  //
  // \param[in] frame 进来的Ethernet帧

  void recv_frame( const EthernetFrame& frame )
  {
    auto optional_dgram = NetworkInterface::recv_frame( frame );
    if ( optional_dgram.has_value() ) {
      datagrams_in_.push( std::move( optional_dgram.value() ) );
    }
  };

  // Access queue of Internet datagrams that have been received
  // 访问已收到的Internet数据报队列

  std::optional<InternetDatagram> maybe_receive()
  {
    if ( datagrams_in_.empty() ) {
      return {};
    }

    InternetDatagram datagram = std::move( datagrams_in_.front() );
    datagrams_in_.pop();
    return datagram;
  }
};

//构建路由信息
struct Route_Message{
  uint32_t route_prefix{};
  uint8_t prefix_length{};
  std::optional<Address> next_hop{};
  size_t interface_num{};
};


// A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
// 一个拥有多个网络接口并在它们之间进行最长前缀匹配路由的路由器。
class Router
{
  // The router's collection of network interfaces
    // 路由器的网络接口集合
  std::vector<AsyncNetworkInterface> interfaces_ {};
   //构建路由映射
  std::vector<Route_Message> route_map {};

public:
  //设置辅助函数

  

  // Add an interface to the router
  // interface: an already-constructed network interface
  // returns the index of the interface after it has been added to the router

  // 向路由器添加一个接口
  // interface: 一个已经构造好的网络接口
  // 返回添加到路由器后的接口索引 
  size_t add_interface( AsyncNetworkInterface&& interface )
  {
    interfaces_.push_back( std::move( interface ) );
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  // 通过索引访问接口
  AsyncNetworkInterface& interface( size_t N ) { return interfaces_.at( N ); }

  // Add a route (a forwarding rule)
  // 添加一条路由（一条转发规则）
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces. For each interface, use the
  // maybe_receive() method to consume every incoming datagram and
  // send it on one of interfaces to the correct next hop. The router
  // chooses the outbound interface and next-hop as specified by the
  // route with the longest prefix_length that matches the datagram's
  // destination address.

  // 在接口之间路由数据报。对于每个接口，使用
  // maybe_receive() 方法消费每个进入的数据报，
  // 并根据路由选择正确的下一跳发送到其中一个接口。路由器
  // 根据与数据报目的地址匹配最长前缀长度的路由规则指定出站接口和下一跳。
  void route();
};

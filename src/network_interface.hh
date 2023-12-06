#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>



// 定义一个结构体来存储MAC地址和存在时间
struct DeviceInfo {
    EthernetAddress macAddress;
    uint64_t existenceTime; // 存在时间，ms为单位
};

struct send_ARPMessages
{
  uint32_t ip_ipv4;
  uint64_t existenceTime;
};

// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
// “网络接口”连接IP（互联网层或网络层）
// 与以太网（网络接入层或链路层）。

// 这个模块是TCP/IP协议栈的最底层
// （将IP与下层网络协议，例如以太网连接起来）。但是，相同的模块也会在路由器中重复使用：
// 一个路由器通常有多个网络接口，路由器的工作是在不同接口之间路由互联网数据报。

// 网络接口将数据报（来自“客户”，例如TCP/IP协议栈或路由器）转换为以太网帧。
// 为了填写以太网目的地址，它会查找每个数据报的下一个IP跳转的以太网地址，
// 使用[地址解析协议](\ref rfc::rfc826)进行请求。
// 在相反的方向上，网络接口接受以太网帧，检查它们是否是为它所设计的，如果是的话，
// 根据其类型处理负载。如果是IPv4数据报，
// 网络接口会将其向上传递。如果是ARP请求或回复，
// 网络接口会处理帧并学习或必要时回复。
// 定义一个结构体来存储MAC地址和存在时间
class NetworkInterface
{
private:
  // Ethernet (known as hardware, network-access, or link-layer) address of the interface
  // 以太网（又称硬件、网络接入层或链路层）接口的地址mac地址
  EthernetAddress ethernet_address_;
  // IP (known as Internet-layer or network-layer) address of the interface
  // IP（又称互联网层或网络层）接口的地址
  Address ip_address_;
  // uint64_t NI_cur_time_;//当前的接口时间
  // uint64_t NI_pre_time_;//之前的接口时间
  uint64_t max_arp_map_existenceTime_=30000;//30秒的最大时间映射
  std::unordered_map<uint32_t, DeviceInfo> arp_map_{}; // ARP映射表，键为IP地址字符串
  std::unordered_map<uint32_t,uint64_t> ethernet_frame_ARPMessage_map_{};//缓解网络堵塞，减少arp风暴
  std::deque<EthernetFrame> ethernet_frame_buffer_deque{};//以太网队列
  std::unordered_map<uint32_t,std::list<InternetDatagram>> InternetDatagram_buffer{};//缓存期待发送的消息

public:
  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  // 使用给定的以太网（网络接入层）和IP（互联网层）地址构造一个网络接口
  NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address );

  // Access queue of Ethernet frames awaiting transmission
  // 访问等待传输的以太网帧队列
  std::optional<EthernetFrame> maybe_send();

  // Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address
  // for the next hop.
  // ("Sending" is accomplished by making sure maybe_send() will release the frame when next called,
  // but please consider the frame sent as soon as it is generated.)
  // 发送一个IPv4数据报，封装在一个以太网帧中（如果它知道以太网目的地址）。
  // 将需要使用[ARP](\ref rfc::rfc826)来查找下一个跳转的以太网目的地址。
  // （“发送”是通过确保maybe_send()在下次调用时会释放帧来完成的，
  // 但请考虑一旦帧生成就认为它已经发送。）
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, returns the datagram.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  // 接收一个以太网帧并作出适当响应。
  // 如果类型是IPv4，返回数据报。
  // 如果类型是ARP请求，从“发送者”字段学习映射，并发送一个ARP回复。
  // 如果类型是ARP回复，从“发送者”字段学习映射。
  std::optional<InternetDatagram> recv_frame( const EthernetFrame& frame );

  // Called periodically when time elapses
  // 定期调用，当时间流逝时
  void tick( size_t ms_since_last_tick );
};

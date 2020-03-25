/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-25 17:32
 * @desc: 基于权重进行路由的路由层协议
 * 路由协议被安放在每个路由器（用作路由的节点）上，对于每个经过的包进行路由选择
 * 
 * - 经典路由规则
 *   路由器拥有一张路由表，对于每个到达的包，检查其源、目的地 (src, dst)，从路由表中读取下一跳，将包发往下一跳  
 *   路由表是通过路由规则在manager中计算好的（实际可能是在本机计算并更新的）  
 *   对于ECMP而言，同样距离的路由有多条，则会随机选择其中一条作为路由，读取这条路由的下一跳  
 * 
 * - 基于权重的路由规则
 *   同样由manager计算路由表，但是路由表项中需要增加一项weight作为选路的权重
 *   类似ECMP，随机选择一条路由发往下一跳。区别在于随机是基于weight随机的
 * 
 */

#ifndef IPV4_RL_ROUTING_H
#define IPV4_RL_ROUTING_H

#include <list>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Ipv4MulticastRoutingTableEntry;
class Node;

/**
 * \ingroup ipv4
 *
 * \brief RL routing protocol for IPv4 stacks.
 *
 * 路由协议被安放在每个路由器（用作路由的节点）上，对于每个经过的包进行路由选择
 * 
 * - 经典路由规则
 *   路由器拥有一张路由表，对于每个到达的包，检查其源、目的地 (src, dst)，从路由表中读取下一跳，将包发往下一跳  
 *   路由表是通过路由规则在manager中计算好的（实际可能是在本机计算并更新的）  
 *   对于ECMP而言，同样距离的路由有多条，则会随机选择其中一条作为路由，读取这条路由的下一跳  
 * 
 * - 基于权重的路由规则
 *   同样由manager计算路由表，但是路由表项中需要增加一项weight作为选路的权重
 *   类似ECMP，随机选择一条路由发往下一跳。区别在于随机是基于weight随机的

 * 
 * - 路由协议不关心，但有意义的部分
 *   通常而言，路由计算只会保留最短的路由。但是对于搭配使用的RLRouting而言：
 *   所有可行路由都会被使用的，只是使用这个路由的概率不同而已。因此不需要比较路由长短，只需要简单判断路由是否可行，
 *   再根据RL决策的路由权重配套下发给路由协议即可
 *
 * This class deals with Ipv4 unicast routes only.
 *
 * \see Ipv4RoutingProtocol
 * \see RLRouteManager
 */
class Ipv4RLRouting : public Ipv4RoutingProtocol
{
public:
  /// 带有权重的路由表（权重用于计算选路概率）
  typedef std::pair<Ipv4RoutingTableEntry *, double> RLHostRoute;
  typedef std::list<RLHostRoute> HostRoutes;
  typedef std::list<RLHostRoute>::const_iterator HostRoutesCI;
  typedef std::list<RLHostRoute>::iterator HostRoutesI;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId(void);
  /**
   * \brief Construct an empty Ipv4RLRouting routing protocol,
   *
   * The Ipv4RLRouting class supports host and network unicast routes.
   * This method initializes the lists containing these routes to empty.
   *
   * \see Ipv4RLRouting
   */
  Ipv4RLRouting();
  virtual ~Ipv4RLRouting();

  // These methods inherited from base class
  virtual Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

  virtual bool RouteInput(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                          UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                          LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp(uint32_t interface);
  virtual void NotifyInterfaceDown(uint32_t interface);
  virtual void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4(Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  /**
   * \brief 添加一条带距离的路由到路由表中
   *
   * \param dest The Ipv4Address destination for this route.
   * \param nextHop The Ipv4Address of the next hop in the route.
   * \param interface The network interface index used to send packets to the
   * destination.
   *
   * \see Ipv4Address
   */
  void AddHostRouteTo(Ipv4Address dest,
                      Ipv4Address nextHop,
                      uint32_t interface,
                      double weight);
  /**
   * \brief Add a host route to the rl routing table.
   *
   * \param dest The Ipv4Address destination for this route.
   * \param nextHop The Ipv4Address of the next hop in the route.
   * \param interface The network interface index used to send packets to the
   * destination.
   *
   * \see Ipv4Address
   */
  void AddHostRouteTo(Ipv4Address dest,
                      Ipv4Address nextHop,
                      uint32_t interface);
  /**
   * \brief Add a host route to the rl routing table.
   *
   * \param dest The Ipv4Address destination for this route.
   * \param interface The network interface index used to send packets to the
   * destination.
   *
   * \see Ipv4Address
   */
  void AddHostRouteTo(Ipv4Address dest,
                      uint32_t interface);

  /**
   * \brief Get the number of individual unicast routes that have been added
   * to the routing table.
   *
   * \warning The default route counts as one of the routes.
   * \returns the number of routes
   */
  uint32_t GetNRoutes(void) const;

  /**
   * \brief Get a route from the rl unicast routing table.
   *
   * Externally, the unicast rl routing table appears simply as a table with
   * n entries.  The one subtlety of note is that if a default route has been set
   * it will appear as the zeroth entry in the table.  This means that if you
   * add only a default route, the table will have one entry that can be accessed
   * either by explicitly calling GetDefaultRoute () or by calling GetRoute (0).
   *
   * Similarly, if the default route has been set, calling RemoveRoute (0) will
   * remove the default route.
   *
   * \param i The index (into the routing table) of the route to retrieve.  If
   * the default route has been set, it will occupy index zero.
   * \return If route is set, a pointer to that Ipv4RoutingTableEntry is returned, otherwise
   * a zero pointer is returned.
   *
   * \see Ipv4RoutingTableEntry
   * \see Ipv4RLRouting::RemoveRoute
   */
  HostRoutesCI GetRoute(uint32_t i) const;

  /**
   * \brief Remove a route from the rl unicast routing table.
   *
   * Externally, the unicast rl routing table appears simply as a table with
   * n entries.  The one subtlety of note is that if a default route has been set
   * it will appear as the zeroth entry in the table.  This means that if the
   * default route has been set, calling RemoveRoute (0) will remove the
   * default route.
   *
   * \param i The index (into the routing table) of the route to remove.  If
   * the default route has been set, it will occupy index zero.
   *
   * \see Ipv4RoutingTableEntry
   * \see Ipv4RLRouting::GetRoute
   * \see Ipv4RLRouting::AddRoute
   */
  void RemoveRoute(uint32_t i);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams(int64_t stream);

protected:
  void DoDispose(void);

private:
  /// Set to true if packets are randomly routed among ECMP; set to false for using only one route consistently
  bool m_randomEcmpRouting;
  /// Set to true if this interface should respond to interface events by rllly recomputing routes
  bool m_respondToInterfaceEvents;
  /// A uniform random number generator for randomly routing packets among ECMP
  Ptr<UniformRandomVariable> m_rand;

  /**
   * \brief Lookup in the forwarding table for destination.
   * \param dest destination address
   * \param oif output interface if any (put 0 otherwise)
   * \param reverse if reverse is true, oif turns to NOT iif
   * \return Ipv4Route to route the packet to reach dest address
   */
  Ptr<Ipv4Route> LookupRL(Ipv4Address dest, uint32_t ifIndex = 0, bool reverse = false);

  HostRoutes m_hostRoutes; //!< Routes to hosts

  Ptr<Ipv4> m_ipv4; //!< associated IPv4 instance
};

} // Namespace ns3

#endif /* IPV4_RL_ROUTING_H */

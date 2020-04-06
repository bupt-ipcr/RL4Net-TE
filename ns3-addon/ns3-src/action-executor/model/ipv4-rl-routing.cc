/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-04-06 16:11
 * @desc: 基于权重进行路由的路由层协议
 */


#include <vector>
#include <iomanip>
#include "ns3/names.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/boolean.h"
#include "ns3/node.h"
#include "ipv4-rl-routing.h"
#include "rl-route-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4RLRouting");

NS_OBJECT_ENSURE_REGISTERED (Ipv4RLRouting);

TypeId 
Ipv4RLRouting::GetTypeId (void)
{ 
  static TypeId tid = TypeId ("ns3::Ipv4RLRouting")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddAttribute ("RandomEcmpRouting",
                   "Set to true if packets are randomly routed among ECMP; set to false for using only one route consistently",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Ipv4RLRouting::m_randomEcmpRouting),
                   MakeBooleanChecker ())
    .AddAttribute ("RespondToInterfaceEvents",
                   "Set to true if you want to dynamically recompute the rl routes upon Interface notification events (up/down, or add/remove address)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Ipv4RLRouting::m_respondToInterfaceEvents),
                   MakeBooleanChecker ())
  ;
  return tid;
}

Ipv4RLRouting::Ipv4RLRouting () 
  : m_randomEcmpRouting (false),
    m_respondToInterfaceEvents (false)
{
  NS_LOG_FUNCTION (this);

  m_rand = CreateObject<UniformRandomVariable> ();
}

Ipv4RLRouting::~Ipv4RLRouting ()
{
  NS_LOG_FUNCTION (this);
}

//新增带有weight（metric）的方法
void 
Ipv4RLRouting::AddHostRouteTo (Ipv4Address dest, 
                                   Ipv4Address nextHop, 
                                   uint32_t interface,
                                   double weight)
{
  NS_LOG_FUNCTION (this << dest << nextHop << interface);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
  *route = Ipv4RoutingTableEntry::CreateHostRouteTo (dest, nextHop, interface);
  RLHostRoute r = RLHostRoute(route, weight);
  m_hostRoutes.push_back (r);
}


void 
Ipv4RLRouting::AddHostRouteTo (Ipv4Address dest, 
                                   Ipv4Address nextHop, 
                                   uint32_t interface)
{
  NS_LOG_FUNCTION (this << dest << nextHop << interface);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
  *route = Ipv4RoutingTableEntry::CreateHostRouteTo (dest, nextHop, interface);
  RLHostRoute r = RLHostRoute(route, 1.0);
  m_hostRoutes.push_back (r);
}

void 
Ipv4RLRouting::AddHostRouteTo (Ipv4Address dest, 
                                   uint32_t interface)
{
  NS_LOG_FUNCTION (this << dest << interface);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
  *route = Ipv4RoutingTableEntry::CreateHostRouteTo (dest, interface);
  RLHostRoute r = RLHostRoute(route, 1.0);
  m_hostRoutes.push_back (r);
}

Ptr<Ipv4Route>
Ipv4RLRouting::LookupRL (Ipv4Address dest, uint32_t ifIndex, bool reverse)
{
  NS_LOG_FUNCTION (this << dest << ifIndex);
  NS_LOG_LOGIC ("Looking for route for destination " << dest);
  Ptr<Ipv4Route> rtentry = 0;
  // store all available routes that bring packets to their destination
  typedef std::vector<RLHostRoute> RouteVec_t;
  RouteVec_t allRoutes;

  NS_LOG_DEBUG ("Number of m_hostRoutes = " << m_hostRoutes.size ());
  for (HostRoutesCI i = m_hostRoutes.begin (); 
       i != m_hostRoutes.end (); 
       i++) 
    {
      // *i 是 RLHostRoute，即<Ipv4RoutingTableEntry *, uint32_t >组成的pair
      Ipv4RoutingTableEntry *r = i->first;
      NS_ASSERT ((r)->IsHost ());
      if ((r)->GetDest () == (dest)) 
        {
          // 如果有outputInterface要求，则检查
          if ( ifIndex != 0){
                if(reverse){
                  if (ifIndex == r->GetInterface ())
                    {
                      NS_LOG_LOGIC ("Banned input interface, skipping");
                      continue;
                    }
                }
                else{
                  if (ifIndex != r->GetInterface ())
                    {
                      NS_LOG_LOGIC ("Not on requested interface, skipping");
                      continue;
                    }
                }
              }
          allRoutes.push_back (*i);
          NS_LOG_LOGIC (allRoutes.size () << "Found rl host route" << r); 
        }
    }
  
  if (allRoutes.size () > 0 ) // if route(s) is found
    {
      uint32_t selectIndex;
      // 按照权重计算概率，确定被选择的路

      // 创建存放概率的变量
      std::vector<double> probVec = std::vector<double>();
    
      // 计算归一化常数（权重之和）
      double weight = 0.0;
      double totalWeight = 0.0;
      NS_LOG_DEBUG("找到 "<<allRoutes.size ()<<"条路由，权重分别为：");
      for(uint32_t index = 0; index < allRoutes.size(); index++){
        weight = allRoutes.at(index).second;
        NS_LOG_DEBUG("  index: "<<index<<"; weight: "<<weight);
        totalWeight += weight;
      }
      NS_LOG_DEBUG("归一化常数："<<totalWeight<<"归一化之后他们的概率分别为：");
      // 归一化
      for(uint32_t index = 0; index < allRoutes.size(); index++){
        weight = allRoutes.at(index).second;
        double prob = weight / totalWeight;
        // 添加到vec中 
        NS_LOG_DEBUG("  index: "<<index<<"; prob: "<<prob);
        probVec.push_back(prob);
      }

      // 获取一个0-1的随机数（区间开闭未知，但是应该没有影响）
      double randValue = m_rand->GetValue(0, 1.0);
      NS_LOG_DEBUG("目标是："<<dest);
      NS_LOG_DEBUG("产生一个随机数："<<randValue);
      // 判断是落在哪里，以此为依据选路
      for (uint32_t index = 0; index < probVec.size(); index ++){
        selectIndex = index;
        randValue = randValue - probVec.at(index);
        if(randValue < 0){
          break;
        }
      }
      NS_LOG_DEBUG("最终选择的index为："<<selectIndex);
        

      Ipv4RoutingTableEntry* route = allRoutes.at(selectIndex).first; 
      NS_LOG_DEBUG("本机IP为："<<m_ipv4->GetAddress (route->GetInterface (), 0).GetLocal ());
      NS_LOG_DEBUG("选择的下一跳ip为："<<route->GetGateway());
      NS_LOG_DEBUG("=============");
      // create a Ipv4Route object from the selected routing table entry
      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (route->GetDest ());
      /// \todo handle multi-address case
      rtentry->SetSource (m_ipv4->GetAddress (route->GetInterface (), 0).GetLocal ());
      rtentry->SetGateway (route->GetGateway ());
      uint32_t interfaceIdx = route->GetInterface ();
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      return rtentry;
    }
  else 
    {
      return 0;
    }
}

uint32_t 
Ipv4RLRouting::GetNRoutes (void) const
{
  NS_LOG_FUNCTION (this);
  uint32_t n = 0;
  n += m_hostRoutes.size ();
  return n;
}

Ipv4RLRouting::HostRoutesCI
Ipv4RLRouting::GetRoute (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  if (index < m_hostRoutes.size ())
    {
      uint32_t tmp = 0;
      for (HostRoutesCI i = m_hostRoutes.begin (); 
           i != m_hostRoutes.end (); 
           i++) 
        {
          if (tmp  == index)
            {
              return i;
            }
          tmp++;
        }
    }
  index -= m_hostRoutes.size ();
  NS_ASSERT (false);
  // quiet compiler.
  return m_hostRoutes.end ();
}
void 
Ipv4RLRouting::RemoveRoute (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  if (index < m_hostRoutes.size ())
    {
      uint32_t tmp = 0;
      for (HostRoutesI i = m_hostRoutes.begin (); 
           i != m_hostRoutes.end (); 
           i++) 
        {
          if (tmp  == index)
            {
              NS_LOG_LOGIC ("Removing route " << index << "; size = " << m_hostRoutes.size ());
              delete i->first;
              m_hostRoutes.erase (i);
              NS_LOG_LOGIC ("Done removing host route " << index << "; host route remaining size = " << m_hostRoutes.size ());
              return;
            }
          tmp++;
        }
    }
  index -= m_hostRoutes.size ();
  NS_ASSERT (false);
}

int64_t
Ipv4RLRouting::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream (stream);
  return 1;
}

void
Ipv4RLRouting::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  for (HostRoutesI i = m_hostRoutes.begin (); 
       i != m_hostRoutes.end (); 
       i = m_hostRoutes.erase (i)) 
    {
      delete (i->first);
    }
  Ipv4RoutingProtocol::DoDispose ();
}

// Formatted like output of "route -n" command
void
Ipv4RLRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  *os << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
      << ", Time: " << Now().As (unit)
      << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
      << ", Ipv4RLRouting table" << std::endl;

  if (GetNRoutes () > 0)
    {
      *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface" << std::endl;
      for (uint32_t j = 0; j < GetNRoutes (); j++)
        {
          std::ostringstream dest, gw, mask, flags;
          Ipv4RoutingTableEntry route = GetRoute (j)->first;
          dest << route.GetDest ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << dest.str ();
          gw << route.GetGateway ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << gw.str ();
          mask << route.GetDestNetworkMask ();
          *os << std::setiosflags (std::ios::left) << std::setw (16) << mask.str ();
          flags << "U";
          if (route.IsHost ())
            {
              flags << "H";
            }
          else if (route.IsGateway ())
            {
              flags << "G";
            }
          *os << std::setiosflags (std::ios::left) << std::setw (6) << flags.str ();
          // Metric not implemented
          *os << "-" << "      ";
          // Ref ct not implemented
          *os << "-" << "      ";
          // Use not implemented
          *os << "-" << "   ";
          if (Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ())) != "")
            {
              *os << Names::FindName (m_ipv4->GetNetDevice (route.GetInterface ()));
            }
          else
            {
              *os << route.GetInterface ();
            }
          *os << std::endl;
        }
    }
  *os << std::endl;
}

Ptr<Ipv4Route>
Ipv4RLRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << p << &header << oif << &sockerr);
//
// First, see if this is a multicast packet we have a route for.  If we
// have a route, then send the packet down each of the specified interfaces.
//
  if (header.GetDestination ().IsMulticast ())
    {
      NS_LOG_LOGIC ("Multicast destination-- returning false");
      return 0; // Let other routing protocols try to handle this
    }
//
// See if this is a unicast packet we have a route for.
//
  NS_LOG_LOGIC ("Unicast destination- looking up");
  Ptr<Ipv4Route> rtentry = LookupRL (header.GetDestination (), oif == 0? 0: oif->GetIfIndex());
  if (rtentry)
    {
      sockerr = Socket::ERROR_NOTERROR;
    }
  else
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }
  return rtentry;
}

bool 
Ipv4RLRouting::RouteInput  (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,                             UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                LocalDeliverCallback lcb, ErrorCallback ecb)
{ 
  NS_LOG_FUNCTION (this << p << header << header.GetSource () << header.GetDestination () << idev << &lcb << &ecb);
  // Check if input device supports IP
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);

  if (m_ipv4->IsDestinationAddress (header.GetDestination (), iif))
    {
      if (!lcb.IsNull ())
        {
          NS_LOG_LOGIC ("Local delivery to " << header.GetDestination ());
          lcb (p, header, iif);
          return true;
        }
      else
        {
          // The local delivery callback is null.  This may be a multicast
          // or broadcast packet, so return false so that another
          // multicast routing protocol can handle it.  It should be possible
          // to extend this to explicitly check whether it is a unicast
          // packet, and invoke the error callback if so
          return false;
        }
    }

  // Check if input device supports IP forwarding
  if (m_ipv4->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return true;
    }
  // Next, try to find a route
  NS_LOG_LOGIC ("Unicast destination- looking up rl route");
  Ptr<Ipv4Route> rtentry = LookupRL (header.GetDestination (), idev->GetIfIndex(), true);
  if (rtentry != 0)
    {
      NS_LOG_LOGIC ("Found unicast destination- calling unicast callback");
      ucb (rtentry, p, header);
      return true;
    }
  else
    {
      NS_LOG_LOGIC ("Did not find unicast destination- returning false");
      return false; // Let other routing protocols try to handle this
                    // route request.
    }
}
void 
Ipv4RLRouting::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  // 20200227 因为RLRouteManager中相应方法删除，及RL路由不需要在插拔时自动重算
  // 屏蔽相应函数的功能
  // if (m_respondToInterfaceEvents && Simulator::Now ().GetSeconds () > 0)  // avoid startup events
  //   {
  //     RLRouteManager::DeleteRLRoutes ();
  //     RLRouteManager::BuildRLRoutingDatabase ();
  //     RLRouteManager::InitializeRoutes ();
  //   }
}

void 
Ipv4RLRouting::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  // 20200227 因为RLRouteManager中相应方法删除，及RL路由不需要在插拔时自动重算
  // 屏蔽相应函数的功能
  // if (m_respondToInterfaceEvents && Simulator::Now ().GetSeconds () > 0)  // avoid startup events
  //   {
  //     RLRouteManager::DeleteRLRoutes ();
  //     RLRouteManager::BuildRLRoutingDatabase ();
  //     RLRouteManager::InitializeRoutes ();
  //   }
}

void 
Ipv4RLRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << address);
  // 20200227 因为RLRouteManager中相应方法删除，及RL路由不需要在插拔时自动重算
  // 屏蔽相应函数的功能
  // if (m_respondToInterfaceEvents && Simulator::Now ().GetSeconds () > 0)  // avoid startup events
  //   {
  //     RLRouteManager::DeleteRLRoutes ();
  //     RLRouteManager::BuildRLRoutingDatabase ();
  //     RLRouteManager::InitializeRoutes ();
  //   }
}

void 
Ipv4RLRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << address);
  // 20200227 因为RLRouteManager中相应方法删除，及RL路由不需要在插拔时自动重算
  // 屏蔽相应函数的功能
  // if (m_respondToInterfaceEvents && Simulator::Now ().GetSeconds () > 0)  // avoid startup events
  //   {
  //     RLRouteManager::DeleteRLRoutes ();
  //     RLRouteManager::BuildRLRoutingDatabase ();
  //     RLRouteManager::InitializeRoutes ();
  //   }
}

void 
Ipv4RLRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_FUNCTION (this << ipv4);
  NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
  m_ipv4 = ipv4;
}


} // namespace ns3

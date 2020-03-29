/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-29 15:55
 * @desc: RL路由协议的接口
 */


#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/channel.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/ipv4.h"
#include "ns3/bridge-net-device.h"
#include "ipv4-rl-routing.h"
#include "rl-router-interface.h"
#include "loopback-net-device.h"
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RLRouter");


// ---------------------------------------------------------------------------
//
// RLRouter Implementation
//
// ---------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (RLRouter);

TypeId 
RLRouter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RLRouter")
    .SetParent<Object> ()
    .SetGroupName ("Internet");
  return tid;
}

RLRouter::RLRouter ()
{
  NS_LOG_FUNCTION (this);
  m_routerId.Set (RLRouteManager::AllocateRouterId ());
}

RLRouter::~RLRouter ()
{
  NS_LOG_FUNCTION (this);
}

void 
RLRouter::SetRoutingProtocol (Ptr<Ipv4RLRouting> routing)
{
  NS_LOG_FUNCTION (this << routing);
  m_routingProtocol = routing;
}
Ptr<Ipv4RLRouting> 
RLRouter::GetRoutingProtocol (void)
{
  NS_LOG_FUNCTION (this);
  return m_routingProtocol;
}

void
RLRouter::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_routingProtocol = 0;
  for (InjectedRoutesI k = m_injectedRoutes.begin ();
       k != m_injectedRoutes.end ();
       k = m_injectedRoutes.erase (k))
    {
      delete (*k);
    }
  Object::DoDispose ();
}

Ipv4Address
RLRouter::GetRouterId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_routerId;
}

NetDeviceContainer
RLRouter::FindAllNonBridgedDevicesOnLink (Ptr<Channel> ch) const
{
  NS_LOG_FUNCTION (this << ch);
  NetDeviceContainer c;

  for (std::size_t i = 0; i < ch->GetNDevices (); i++)
    {
      Ptr<NetDevice> nd = ch->GetDevice (i);
      NS_LOG_LOGIC ("checking to see if the device " << nd << " is bridged");
      Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged (nd);
      if (bnd && BridgeHasAlreadyBeenVisited (bnd) == false)
        {
          NS_LOG_LOGIC ("Device is bridged by BridgeNetDevice " << bnd << " with " << bnd->GetNBridgePorts () << " ports");
          MarkBridgeAsVisited (bnd);
          // Find all channels bridged together, and recursively call
          // on all other channels
          for (uint32_t j = 0; j < bnd->GetNBridgePorts (); j++)
            {
              Ptr<NetDevice> bridgedDevice = bnd->GetBridgePort (j);
              if (bridgedDevice->GetChannel () == ch)
                {
                  NS_LOG_LOGIC ("Skipping my own device/channel");
                  continue;
                }
              NS_LOG_LOGIC ("Calling on channel " << bridgedDevice->GetChannel ());
              c.Add (FindAllNonBridgedDevicesOnLink (bridgedDevice->GetChannel ()));
            }
        }
      else
        {
          NS_LOG_LOGIC ("Device is not bridged; adding");
          c.Add (nd);
        }
    }
  NS_LOG_LOGIC ("Found " << c.GetN () << " devices");
  return c;
}

//
// Given a local net device, we need to walk the channel to which the net device is
// attached and look for nodes with RLRouter interfaces on them (one of them 
// will be us).  Of these, the router with the lowest IP address on the net device 
// connecting to the channel becomes the designated router for the link.
//
Ipv4Address
RLRouter::FindDesignatedRouterForLink (Ptr<NetDevice> ndLocal) const
{
  NS_LOG_FUNCTION (this << ndLocal);

  Ptr<Channel> ch = ndLocal->GetChannel ();
  uint32_t nDevices = ch->GetNDevices ();
  NS_ASSERT (nDevices);

  NS_LOG_LOGIC ("Looking for designated router off of net device " << ndLocal << " on node " << 
                ndLocal->GetNode ()->GetId ());

  Ipv4Address desigRtr ("255.255.255.255");

  //
  // Look through all of the devices on the channel to which the net device
  // in question is attached.
  //
  for (uint32_t i = 0; i < nDevices; i++)
    {
      Ptr<NetDevice> ndOther = ch->GetDevice (i);
      NS_ASSERT (ndOther);

      Ptr<Node> nodeOther = ndOther->GetNode ();

      NS_LOG_LOGIC ("Examine channel device " << i << " on node " << nodeOther->GetId ());

      //
      // For all other net devices, we need to check and see if a router
      // is present.  If the net device on the other side is a bridged
      // device, we need to consider all of the other devices on the 
      // bridge as well (all of the bridge ports.
      //
      NS_LOG_LOGIC ("checking to see if the device is bridged");
      Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged (ndOther);
      if (bnd)
        {
          NS_LOG_LOGIC ("Device is bridged by BridgeNetDevice " << bnd);

          //
          // When enumerating a bridge, don't count the netdevice we came in on
          //
          if (ndLocal == ndOther)
            {
              NS_LOG_LOGIC ("Skip -- it is where we came from.");
              continue;
            }

          //
          // It is possible that the bridge net device is sitting under a
          // router, so we have to check for the presence of that router
          // before we run off and follow all the links
          //
          // We require a designated router to have a RLRouter interface and
          // an internet stack that includes the Ipv4 interface.  If it doesn't
          // it can't play router.
          //
          NS_LOG_LOGIC ("Checking for router on bridge net device " << bnd);
          Ptr<RLRouter> rtr = nodeOther->GetObject<RLRouter> ();
          Ptr<Ipv4> ipv4 = nodeOther->GetObject<Ipv4> ();
          if (rtr && ipv4)
            {
              // Initialize to value out of bounds to silence compiler
              uint32_t interfaceOther = ipv4->GetNInterfaces () + 1;
              if (FindInterfaceForDevice (nodeOther, bnd, interfaceOther))
                {
                  NS_LOG_LOGIC ("Found router on bridge net device " << bnd);
                  if (!ipv4->IsUp (interfaceOther))
                    {
                      NS_LOG_LOGIC ("Remote side interface " << interfaceOther << " not up");
                      continue;
                    }
                  if (ipv4->GetNAddresses (interfaceOther) > 1)
                    {
                      NS_LOG_WARN ("Warning, interface has multiple IP addresses; using only the primary one");
                    }
                  Ipv4Address addrOther = ipv4->GetAddress (interfaceOther, 0).GetLocal ();
                  desigRtr = addrOther < desigRtr ? addrOther : desigRtr;
                  NS_LOG_LOGIC ("designated router now " << desigRtr);
                }
            }

          // 
          // Check if we have seen this bridge net device already while
          // recursively enumerating an L2 broadcast domain. If it is new 
          // to us, go ahead and process it. If we have already processed it,
          // move to the next
          // 
          if(BridgeHasAlreadyBeenVisited(bnd))
            {
              NS_ABORT_MSG ("ERROR: L2 forwarding loop detected!");
            }

          MarkBridgeAsVisited(bnd);

          NS_LOG_LOGIC ("Looking through bridge ports of bridge net device " << bnd);
          for (uint32_t j = 0; j < bnd->GetNBridgePorts (); ++j)
            {
              Ptr<NetDevice> ndBridged = bnd->GetBridgePort (j);
              NS_LOG_LOGIC ("Examining bridge port " << j << " device " << ndBridged);
              if (ndBridged == ndOther)
                {
                  NS_LOG_LOGIC ("That bridge port is me, don't walk backward");
                  continue;
                }

              NS_LOG_LOGIC ("Recursively looking for routers down bridge port " << ndBridged);
              Ipv4Address addrOther = FindDesignatedRouterForLink (ndBridged);
              desigRtr = addrOther < desigRtr ? addrOther : desigRtr;
              NS_LOG_LOGIC ("designated router now " << desigRtr);
            }
        }
      else
        {
          NS_LOG_LOGIC ("This device is not bridged");
          Ptr<Node> nodeOther = ndOther->GetNode ();
          NS_ASSERT (nodeOther);

          //
          // We require a designated router to have a RLRouter interface and
          // an internet stack that includes the Ipv4 interface.  If it doesn't
          //
          Ptr<RLRouter> rtr = nodeOther->GetObject<RLRouter> ();
          Ptr<Ipv4> ipv4 = nodeOther->GetObject<Ipv4> ();
          if (rtr && ipv4)
            {
              // Initialize to value out of bounds to silence compiler
              uint32_t interfaceOther = ipv4->GetNInterfaces () + 1;
              if (FindInterfaceForDevice (nodeOther, ndOther, interfaceOther))
                {
                  if (!ipv4->IsUp (interfaceOther))
                    {
                      NS_LOG_LOGIC ("Remote side interface " << interfaceOther << " not up");
                      continue;
                    }
                  NS_LOG_LOGIC ("Found router on net device " << ndOther);
                  if (ipv4->GetNAddresses (interfaceOther) > 1)
                    {
                      NS_LOG_WARN ("Warning, interface has multiple IP addresses; using only the primary one");
                    }
                  Ipv4Address addrOther = ipv4->GetAddress (interfaceOther, 0).GetLocal ();
                  desigRtr = addrOther < desigRtr ? addrOther : desigRtr;
                  NS_LOG_LOGIC ("designated router now " << desigRtr);
                }
            }
        }
    }
  return desigRtr;
}

//
// Given a node and an attached net device, take a look off in the channel to 
// which the net device is attached and look for a node on the other side
// that has a RLRouter interface aggregated.  Life gets more complicated
// when there is a bridged net device on the other side.
//
bool
RLRouter::AnotherRouterOnLink (Ptr<NetDevice> nd) const
{
  NS_LOG_FUNCTION (this << nd);

  Ptr<Channel> ch = nd->GetChannel ();
  if (!ch)
    {
      // It may be that this net device is a stub device, without a channel
      return false;
    }
  uint32_t nDevices = ch->GetNDevices ();
  NS_ASSERT (nDevices);

  NS_LOG_LOGIC ("Looking for routers off of net device " << nd << " on node " << nd->GetNode ()->GetId ());

  //
  // Look through all of the devices on the channel to which the net device
  // in question is attached.
  //
  for (uint32_t i = 0; i < nDevices; i++)
    {
      Ptr<NetDevice> ndOther = ch->GetDevice (i);
      NS_ASSERT (ndOther);

      NS_LOG_LOGIC ("Examine channel device " << i << " on node " << ndOther->GetNode ()->GetId ());

      // 
      // Ignore the net device itself.
      //
      if (ndOther == nd)
        {
          NS_LOG_LOGIC ("Myself, skip");
          continue;
        }

      //
      // For all other net devices, we need to check and see if a router
      // is present.  If the net device on the other side is a bridged
      // device, we need to consider all of the other devices on the 
      // bridge.
      //
      NS_LOG_LOGIC ("checking to see if device is bridged");
      Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged (ndOther);
      if (bnd)
        {
          NS_LOG_LOGIC ("Device is bridged by net device " << bnd);

          // 
          // Check if we have seen this bridge net device already while
          // recursively enumerating an L2 broadcast domain. If it is new 
          // to us, go ahead and process it. If we have already processed it,
          // move to the next
          // 
          if(BridgeHasAlreadyBeenVisited(bnd))
            {
              NS_ABORT_MSG ("ERROR: L2 forwarding loop detected!");
            }

          MarkBridgeAsVisited(bnd);

          NS_LOG_LOGIC ("Looking through bridge ports of bridge net device " << bnd);
          for (uint32_t j = 0; j < bnd->GetNBridgePorts (); ++j)
            {
              Ptr<NetDevice> ndBridged = bnd->GetBridgePort (j);
              NS_LOG_LOGIC ("Examining bridge port " << j << " device " << ndBridged);
              if (ndBridged == ndOther)
                {
                  NS_LOG_LOGIC ("That bridge port is me, skip");
                  continue;
                }

              NS_LOG_LOGIC ("Recursively looking for routers on bridge port " << ndBridged);
              if (AnotherRouterOnLink (ndBridged))
                {
                  NS_LOG_LOGIC ("Found routers on bridge port, return true");
                  return true;
                }
            }
          NS_LOG_LOGIC ("No routers on bridged net device, return false");
          return false;
        }

      NS_LOG_LOGIC ("This device is not bridged");
      Ptr<Node> nodeTemp = ndOther->GetNode ();
      NS_ASSERT (nodeTemp);

      Ptr<RLRouter> rtr = nodeTemp->GetObject<RLRouter> ();
      if (rtr)
        {
          NS_LOG_LOGIC ("Found RLRouter interface, return true");
          return true;
        }
      else 
        {
          NS_LOG_LOGIC ("No RLRouter interface on device, continue search");
        }
    }
  NS_LOG_LOGIC ("No routers found, return false");
  return false;
}

void
RLRouter::InjectRoute (Ipv4Address network, Ipv4Mask networkMask)
{
  NS_LOG_FUNCTION (this << network << networkMask);
  Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry ();
//
// Interface number does not matter here, using 1.
//
  *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo (network,
                                                        networkMask,
                                                        1);
  m_injectedRoutes.push_back (route);
}

Ipv4RoutingTableEntry *
RLRouter::GetInjectedRoute (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  if (index < m_injectedRoutes.size ())
    {
      uint32_t tmp = 0;
      for (InjectedRoutesCI i = m_injectedRoutes.begin ();
           i != m_injectedRoutes.end ();
           i++)
        {
          if (tmp  == index)
            {
              return *i;
            }
          tmp++;
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return 0;
}

uint32_t
RLRouter::GetNInjectedRoutes ()
{
  NS_LOG_FUNCTION (this);
  return m_injectedRoutes.size ();
}

void
RLRouter::RemoveInjectedRoute (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  NS_ASSERT (index < m_injectedRoutes.size ());
  uint32_t tmp = 0;
  for (InjectedRoutesI i = m_injectedRoutes.begin (); i != m_injectedRoutes.end (); i++)
    {
      if (tmp  == index)
        {
          NS_LOG_LOGIC ("Removing route " << index << "; size = " << m_injectedRoutes.size ());
          delete *i;
          m_injectedRoutes.erase (i);
          return;
        }
      tmp++;
    }
}

bool
RLRouter::WithdrawRoute (Ipv4Address network, Ipv4Mask networkMask)
{
  NS_LOG_FUNCTION (this << network << networkMask);
  for (InjectedRoutesI i = m_injectedRoutes.begin (); i != m_injectedRoutes.end (); i++)
    {
      if ((*i)->GetDestNetwork () == network && (*i)->GetDestNetworkMask () == networkMask)
        {
          NS_LOG_LOGIC ("Withdrawing route to network/mask " << network << "/" << networkMask);
          delete *i;
          m_injectedRoutes.erase (i);
          return true;
        }
    }
  return false;
}


//
// Link through the given channel and find the net device that's on the
// other end.  This only makes sense with a point-to-point channel.
//
Ptr<NetDevice>
RLRouter::GetAdjacent (Ptr<NetDevice> nd, Ptr<Channel> ch) const
{
  NS_LOG_FUNCTION (this << nd << ch);
  NS_ASSERT_MSG (ch->GetNDevices () == 2, "RLRouter::GetAdjacent (): Channel with other than two devices");
//
// This is a point to point channel with two endpoints.  Get both of them.
//
  Ptr<NetDevice> nd1 = ch->GetDevice (0);
  Ptr<NetDevice> nd2 = ch->GetDevice (1);
//
// One of the endpoints is going to be "us" -- that is the net device attached
// to the node on which we're running -- i.e., "nd".  The other endpoint (the
// one to which we are connected via the channel) is the adjacent router.
//
  if (nd1 == nd)
    {
      return nd2;
    }
  else if (nd2 == nd)
    {
      return nd1;
    }
  else
    {
      NS_ASSERT_MSG (false,
                     "RLRouter::GetAdjacent (): Wrong or confused channel?");
      return 0;
    }
}

//
// Given a node and a net device, find an IPV4 interface index that corresponds
// to that net device.  This function may fail for various reasons.  If a node
// does not have an internet stack (for example if it is a bridge) we won't have
// an IPv4 at all.  If the node does have a stack, but the net device in question
// is bridged, there will not be an interface associated directly with the device.
//
bool
RLRouter::FindInterfaceForDevice (Ptr<Node> node, Ptr<NetDevice> nd, uint32_t &index) const
{
  NS_LOG_FUNCTION (this << node << nd << &index);
  NS_LOG_LOGIC ("For node " << node->GetId () << " for net device " << nd );

  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  if (ipv4 == 0)
    {
      NS_LOG_LOGIC ("No Ipv4 interface on node " << node->GetId ());
      return false;
    }

  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); ++i )
    {
      if (ipv4->GetNetDevice (i) == nd)
        {
          NS_LOG_LOGIC ("Device " << nd << " has associated ipv4 index " << i);
          index = i;
          return true;
        }
    }

  NS_LOG_LOGIC ("Device " << nd << " has no associated ipv4 index");
  return false;
}

//
// Decide whether or not a given net device is being bridged by a BridgeNetDevice.
//
Ptr<BridgeNetDevice>
RLRouter::NetDeviceIsBridged (Ptr<NetDevice> nd) const
{
  NS_LOG_FUNCTION (this << nd);

  Ptr<Node> node = nd->GetNode ();
  uint32_t nDevices = node->GetNDevices ();

  //
  // There is no bit on a net device that says it is being bridged, so we have
  // to look for bridges on the node to which the device is attached.  If we
  // find a bridge, we need to look through its bridge ports (the devices it
  // bridges) to see if we find the device in question.
  //
  for (uint32_t i = 0; i < nDevices; ++i)
    {
      Ptr<NetDevice> ndTest = node->GetDevice (i);
      NS_LOG_LOGIC ("Examine device " << i << " " << ndTest);

      if (ndTest->IsBridge ())
        {
          NS_LOG_LOGIC ("device " << i << " is a bridge net device");
          Ptr<BridgeNetDevice> bnd = ndTest->GetObject<BridgeNetDevice> ();
          NS_ABORT_MSG_UNLESS (bnd, "RLRouter::DiscoverLSAs (): GetObject for <BridgeNetDevice> failed");

          for (uint32_t j = 0; j < bnd->GetNBridgePorts (); ++j)
            {
              NS_LOG_LOGIC ("Examine bridge port " << j << " " << bnd->GetBridgePort (j));
              if (bnd->GetBridgePort (j) == nd)
                {
                  NS_LOG_LOGIC ("Net device " << nd << " is bridged by " << bnd);
                  return bnd;
                }
            }
        }
    }
  NS_LOG_LOGIC ("Net device " << nd << " is not bridged");
  return 0;
}

//
// Start a new enumeration of an L2 broadcast domain by clearing m_bridgesVisited
//
void 
RLRouter::ClearBridgesVisited (void) const
{
  m_bridgesVisited.clear();
}

//
// Check if we have already visited a given bridge net device by searching m_bridgesVisited
//
bool
RLRouter::BridgeHasAlreadyBeenVisited (Ptr<BridgeNetDevice> bridgeNetDevice) const
{
  std::vector<Ptr<BridgeNetDevice> >::iterator iter;
  for (iter = m_bridgesVisited.begin (); iter != m_bridgesVisited.end (); ++iter)
    {
      if (bridgeNetDevice == *iter)
        {
          NS_LOG_LOGIC ("Bridge " << bridgeNetDevice << " has been visited.");
          return true;
        }
    }
  return false;
}

//
// Remember that we visited a bridge net device by adding it to m_bridgesVisited
//
void 
RLRouter::MarkBridgeAsVisited (Ptr<BridgeNetDevice> bridgeNetDevice) const
{
  NS_LOG_FUNCTION (this << bridgeNetDevice);
  m_bridgesVisited.push_back (bridgeNetDevice);
}


} // namespace ns3

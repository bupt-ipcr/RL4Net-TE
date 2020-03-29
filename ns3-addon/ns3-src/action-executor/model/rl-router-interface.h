/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-29 15:56
 * @desc: RL路由协议的接口
 */


#ifndef RL_ROUTER_INTERFACE_H
#define RL_ROUTER_INTERFACE_H

#include <stdint.h>
#include <list>
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/ipv4-address.h"
#include "ns3/net-device-container.h"
#include "ns3/bridge-net-device.h"
#include "ns3/rl-route-manager.h"
#include "ns3/ipv4-routing-table-entry.h"

namespace ns3 {

class RLRouter;
class Ipv4RLRouting;

/**
 * @brief An interface aggregated to a node to provide rl routing info
 *
 * An interface aggregated to a node that provides rl routing information
 * to a rl route manager.  The presence of the interface indicates that
 * the node is a router.  The interface is the mechanism by which the router
 * advertises its connections to neighboring routers.  We're basically 
 * allowing the route manager to query for link state advertisements.
 */
class RLRouter : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

/**
 * @brief Create a RL Router class 
 */
  RLRouter ();

  /**
   * \brief Set the specific RL Routing Protocol to be used
   * \param routing the routing protocol
   */
  void SetRoutingProtocol (Ptr<Ipv4RLRouting> routing);

  /**
   * \brief Get the specific RL Routing Protocol used
   * \returns the routing protocol
   */
  Ptr<Ipv4RLRouting> GetRoutingProtocol (void);

/**
 * @brief Get the Router ID associated with this RL Router.
 *
 * The Router IDs are allocated in the RoutingEnvironment -- one per Router, 
 * starting at 0.0.0.1 and incrementing with each instantiation of a router.
 *
 * @see RoutingEnvironment::AllocateRouterId ()
 * @returns The Router ID associated with the RL Router.
 */
  Ipv4Address GetRouterId (void) const;

/**
 * @brief Inject a route to be circulated to other routers as an external
 * route
 *
 * @param network The Network to inject
 * @param networkMask The Network Mask to inject
 */
  void InjectRoute (Ipv4Address network, Ipv4Mask networkMask);

/**
 * @brief Get the number of injected routes that have been added
 * to the routing table.
 * @return number of injected routes
 */
  uint32_t GetNInjectedRoutes (void);

/**
 * @brief Return the injected route indexed by i
 * @param i the index of the route
 * @return a pointer to that Ipv4RoutingTableEntry is returned
 *
 */
  Ipv4RoutingTableEntry *GetInjectedRoute (uint32_t i);

/**
 * @brief Withdraw a route from the rl unicast routing table.
 *
 * Calling this function will cause all indexed routes numbered above
 * index i to have their index decremented.  For instance, it is possible to
 * remove N injected routes by calling RemoveInjectedRoute (0) N times.
 *
 * @param i The index (into the injected routing list) of the route to remove.
 *
 * @see RLRouter::WithdrawRoute ()
 */
  void RemoveInjectedRoute (uint32_t i);

/**
 * @brief Withdraw a route from the rl unicast routing table.
 *
 * @param network The Network to withdraw
 * @param networkMask The Network Mask to withdraw
 * @return whether the operation succeeded (will return false if no such route)
 *
 * @see RLRouter::RemoveInjectedRoute ()
 */
  bool WithdrawRoute (Ipv4Address network, Ipv4Mask networkMask);

private:
  virtual ~RLRouter ();

  /**
   * \brief Link through the given channel and find the net device that's on the other end.
   *
   * This only makes sense with a point-to-point channel.
   *
   * \param nd outgoing NetDevice
   * \param ch channel
   * \returns the NetDevice on the other end
   */
  Ptr<NetDevice> GetAdjacent (Ptr<NetDevice> nd, Ptr<Channel> ch) const;

  /**
   * \brief Given a node and a net device, find an IPV4 interface index that corresponds
   *        to that net device.
   *
   * This function may fail for various reasons.  If a node
   * does not have an internet stack (for example if it is a bridge) we won't have
   * an IPv4 at all.  If the node does have a stack, but the net device in question
   * is bridged, there will not be an interface associated directly with the device.
   *
   * \param node the node
   * \param nd outgoing NetDevice
   * \param index the IPV4 interface index
   * \returns true on success
   */
  bool FindInterfaceForDevice (Ptr<Node> node, Ptr<NetDevice> nd, uint32_t &index) const;

  /**
   * \brief Finds a designated router
   *
   * Given a local net device, we need to walk the channel to which the net device is
   * attached and look for nodes with RLRouter interfaces on them (one of them
   * will be us).  Of these, the router with the lowest IP address on the net device
   * connecting to the channel becomes the designated router for the link.
   *
   * \param ndLocal local NetDevice to scan
   * \returns the IP address of the designated router
   */
  Ipv4Address FindDesignatedRouterForLink (Ptr<NetDevice> ndLocal) const;

  /**
   * \brief Checks for the presence of another router on the NetDevice
   *
   * Given a node and an attached net device, take a look off in the channel to
   * which the net device is attached and look for a node on the other side
   * that has a RLRouter interface aggregated.  
   *
   * \param nd NetDevice to scan
   * \returns true if a router is found
   */
  bool AnotherRouterOnLink (Ptr<NetDevice> nd) const;

  /**
   * \brief Return a container of all non-bridged NetDevices on a link
   *
   * This method will recursively find all of the 'edge' devices in an
   * L2 broadcast domain.  If there are no bridged devices, then the
   * container returned is simply the set of devices on the channel
   * passed in as an argument.  If the link has bridges on it 
   * (and therefore multiple ns3::Channel objects interconnected by 
   * bridges), the method will find all of the non-bridged devices
   * in the L2 broadcast domain.
   *
   * \param ch a channel from the link
   * \returns the NetDeviceContainer.
   */
  NetDeviceContainer FindAllNonBridgedDevicesOnLink (Ptr<Channel> ch) const;

  /**
   * \brief Decide whether or not a given net device is being bridged by a BridgeNetDevice.
   *
   * \param nd the NetDevice
   * \returns the BridgeNetDevice smart pointer or null if not found
   */
  Ptr<BridgeNetDevice> NetDeviceIsBridged (Ptr<NetDevice> nd) const;

  Ipv4Address m_routerId; //!< router ID (its IPv4 address)
  Ptr<Ipv4RLRouting> m_routingProtocol; //!< the Ipv4RLRouting in use

  typedef std::list<Ipv4RoutingTableEntry *> InjectedRoutes; //!< container of Ipv4RoutingTableEntry
  typedef std::list<Ipv4RoutingTableEntry *>::const_iterator InjectedRoutesCI; //!< Const Iterator to container of Ipv4RoutingTableEntry
  typedef std::list<Ipv4RoutingTableEntry *>::iterator InjectedRoutesI; //!< Iterator to container of Ipv4RoutingTableEntry
  InjectedRoutes m_injectedRoutes; //!< Routes we are exporting

  // Declared mutable so that const member functions can clear it
  // (supporting the logical constness of the search methods of this class) 
  /**
   * Container of bridges visited.
   */
  mutable std::vector<Ptr<BridgeNetDevice> > m_bridgesVisited;
  /**
   * Clear the list of bridges visited on the link 
   */
  void ClearBridgesVisited (void) const;
  /**
   * When recursively checking for devices on the link, check whether a
   * given device has already been visited.
   *
   * \param device the bridge device to check
   * \return true if bridge has already been visited 
   */
  bool BridgeHasAlreadyBeenVisited (Ptr<BridgeNetDevice> device) const;
  /**
   * When recursively checking for devices on the link, mark a given device 
   * as having been visited.
   *
   * \param device the bridge device to mark
   */
  void MarkBridgeAsVisited (Ptr<BridgeNetDevice> device) const;

  // inherited from Object
  virtual void DoDispose (void);

/**
 * @brief RL Router copy construction is disallowed.
 * @param sr object to copy from.
 */
  RLRouter (RLRouter& sr);

/**
 * @brief RL Router assignment operator is disallowed.
 * @param sr object to copy from.
 * @returns The object copied.
 */
  RLRouter& operator= (RLRouter& sr);
};

} // namespace ns3

#endif /* RL_ROUTER_INTERFACE_H */

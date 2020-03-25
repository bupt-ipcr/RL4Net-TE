/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-25 17:00
 * @desc: 封装RL路由，对外提供初始化和计算路由表的功能
 */

#include "ipv4-rl-routing-helper.h"
#include "ns3/rl-router-interface.h"
#include "ns3/ipv4-rl-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RLRoutingHelper");

Ipv4RLRoutingHelper::Ipv4RLRoutingHelper ()
{
}

Ipv4RLRoutingHelper::Ipv4RLRoutingHelper (const Ipv4RLRoutingHelper &o)
{
}

Ipv4RLRoutingHelper*
Ipv4RLRoutingHelper::Copy (void) const
{
  return new Ipv4RLRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4RLRoutingHelper::Create (Ptr<Node> node) const
{
  NS_LOG_LOGIC ("Adding RLRouter interface to node " <<
                node->GetId ());

  Ptr<RLRouter> rLRouter = CreateObject<RLRouter> ();
  node->AggregateObject (rLRouter);

  NS_LOG_LOGIC ("Adding RLRouting Protocol to node " << node->GetId ());
  Ptr<Ipv4RLRouting> rLRouting = CreateObject<Ipv4RLRouting> ();
  rLRouter->SetRoutingProtocol (rLRouting);

  return rLRouting;
}

void 
Ipv4RLRoutingHelper::InitializeRouteDatabase(int * adjacencyArray, NodeContainer nodes)
{
  RLRouteManager::BuildRLRoutingDatabase (adjacencyArray, nodes);
}

void 
Ipv4RLRoutingHelper::ComputeRoutingTables (double *weightArray)
{
  RLRouteManager::DeleteRoutes();
  RLRouteManager::SetWeightMatrix (weightArray);
  RLRouteManager::CalculateRoutes ();
}

} // namespace ns3

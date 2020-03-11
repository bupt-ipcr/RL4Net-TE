/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-02 19:40
 * @FilePath: /ns3src/internet/helper/ipv4-rl-routing-helper.cc
 */
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
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

// 重载函数，允许添加metric信息
void 
Ipv4RLRoutingHelper::ComputeRoutingTables (double *weightArray)
{
  RLRouteManager::DeleteRoutes();
  RLRouteManager::SetWeightMatrix (weightArray);
  RLRouteManager::CalculateRoutes ();
}

} // namespace ns3

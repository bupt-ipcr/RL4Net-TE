/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-02-28 16:34
 * @FilePath: /ns3src/internet/helper/ipv4-rl-routing-helper.h
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
#ifndef IPV4_RL_ROUTING_HELPER_H
#define IPV4_RL_ROUTING_HELPER_H

#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

/**
 * \ingroup ipv4Helpers
 *
 * \brief Helper class that adds ns3::Ipv4RLRouting objects
 */
class Ipv4RLRoutingHelper  : public Ipv4RoutingHelper
{
public:
  /**
   * \brief Construct a RLRoutingHelper to make life easier for managing
   * rl routing tasks.
   */
  Ipv4RLRoutingHelper ();

  /**
   * \brief Construct a RLRoutingHelper from another previously initialized
   * instance (Copy Constructor).
   */
  Ipv4RLRoutingHelper (const Ipv4RLRoutingHelper &);

  /**
   * \returns pointer to clone of this Ipv4RLRoutingHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv4RLRoutingHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  /**
   * @brief 初始化路由表
   * 
   * 通过RLRouteManager类的BuildRLRoutingDatabase方法初始化数据库
   * 
   * @param adjacencyArray 
   * @param nodes
   */
  static void InitializeRouteDatabase(int * adjacencyArray, NodeContainer nodes);

    /**
   * \brief 传入metrix矩阵，要求重新计算routingtable
   *
   * All this function does is call the functions
   * BuildRLRoutingDatabase () and  InitializeRoutes () and DeleteRLRoutes().
   * 和 SetMetricMatrix()
   *
   */
  static void ComputeRoutingTables (double *metricArray);
private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   * \return
   */
  Ipv4RLRoutingHelper &operator = (const Ipv4RLRoutingHelper &);
};

} // namespace ns3

#endif /* IPV4_RL_ROUTING_HELPER_H */

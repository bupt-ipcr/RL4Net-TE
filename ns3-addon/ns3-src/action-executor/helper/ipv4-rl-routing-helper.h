/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-25 17:01
 * @desc: 封装RL路由，对外提供初始化和计算路由表的功能
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

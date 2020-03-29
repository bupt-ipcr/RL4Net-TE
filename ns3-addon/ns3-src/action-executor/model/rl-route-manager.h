
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * @author: Jiawei Wu
 * @create time: 2020-03-02 19:42
 * @edit time: 2020-03-25 17:36
 * @desc: RL概率路由计算的抽象类
 */

#include "ns3/network-module.h"
#ifndef RL_ROUTE_MANAGER_H
#define RL_ROUTE_MANAGER_H

namespace ns3 {

/**
 * \ingroup rlrouting
 *
 * @brief A rl rl router
 *
 * This singleton object can query interface each node in the system
 * for a RLRouter interface.  For those nodes, it fetches one or
 * more Link State Advertisements and stores them in a local database.
 * Then, it can compute shortest paths on a per-node basis to all routers, 
 * and finally configure each of the node's forwarding tables.
 *
 * The design is guided by OSPFv2 \RFC{2328} section 16.1.1 and quagga ospfd.
 */
class RLRouteManager
{
public:
  /**
 * @brief Allocate a 32-bit router ID from monotonically increasing counter.
 * @returns A new new RouterId.
 */
  static uint32_t AllocateRouterId ();

  /**
   * @brief 删除所有路由的抽象方法
   * 
   * 每次重新计算路由之前应该先清空路由表
   */
  static void DeleteRoutes ();
  
  /**
   * @brief 构建RL数据库的抽象方法
   * 
   * @param adjacencyArray 
   * @param nodes 
   */
  static void BuildRLRoutingDatabase (int *adjacencyArray, NodeContainer nodes);

  /**
   * @brief 设置权重矩阵
   * 
   * @param weightArray 
   */
  static void SetWeightMatrix (double *weightArray);

  /**
    * @brief 计算并下发路由表
    * 
    */
  static void CalculateRoutes ();

private:
  /**
 * @brief RL Route Manager copy construction is disallowed.  There's no 
 * need for it and a compiler provided shallow copy would be wrong.
 *
 * @param srm object to copy from
 */
  RLRouteManager (RLRouteManager &srm);

  /**
 * @brief RL Router copy assignment operator is disallowed.  There's no 
 * need for it and a compiler provided shallow copy would be wrong.
 *
 * @param srm object to copy from
 * @returns the copied object
 */
  RLRouteManager &operator= (RLRouteManager &srm);
};

} // namespace ns3

#endif /* RL_ROUTE_MANAGER_H */

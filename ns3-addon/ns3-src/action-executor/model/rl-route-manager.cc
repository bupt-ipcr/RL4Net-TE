/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-02 19:43
 * @desc: RL概率路由计算的抽象类
 */
 

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulation-singleton.h"
#include "rl-route-manager.h"
#include "rl-route-manager-impl.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RLRouteManager");

// ---------------------------------------------------------------------------
//
// RLRouteManager Implementation
//
// ---------------------------------------------------------------------------
void
RLRouteManager::DeleteRoutes (){
  NS_LOG_FUNCTION_NOARGS ();
  SimulationSingleton<RLRouteManagerImpl>::Get ()->
  DeleteRoutes ();
}

void
RLRouteManager::BuildRLRoutingDatabase (int *adjacencyArray, NodeContainer nodes)
{
  NS_LOG_FUNCTION_NOARGS ();
  SimulationSingleton<RLRouteManagerImpl>::Get ()->
  BuildRLRoutingDatabase (adjacencyArray, nodes);
}

void
RLRouteManager::SetWeightMatrix (double *weightArray)
{
  NS_LOG_FUNCTION_NOARGS ();
  SimulationSingleton<RLRouteManagerImpl>::Get ()->
  SetWeightMatrix (weightArray);
}

void
RLRouteManager::CalculateRoutes ()
{
  NS_LOG_FUNCTION_NOARGS ();
  SimulationSingleton<RLRouteManagerImpl>::Get ()->
  CalculateRoutes ();
}

uint32_t
RLRouteManager::AllocateRouterId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static uint32_t routerId = 0;
  return routerId++;
}


} // namespace ns3

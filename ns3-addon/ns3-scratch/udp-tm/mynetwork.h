/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-01 22:43
 * @FilePath: /simulator/udp-tm/mynetwork.h
 */

#ifndef MY_NETWORK_H
#define MY_NETWORK_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"

namespace ns3 {
class MyNetwork : public Object
{
public:
  typedef std::pair<uint32_t, uint32_t> NodePair; //!< 两个node（由index确定）可以确定一条有向边
  typedef std::pair<uint32_t, uint32_t>
      NodeInterfacePair; //!< 两个node（由index确定）可以确定一条Flow
  typedef std::map<NodeInterfacePair, NodePair> ChannelMap; //!< 两种确定边的方式构成Map
  typedef std::vector<NodePair> FlowVec; //!< 记录FlowId和对应的node的关系的Vector
  
  MyNetwork ();
  MyNetwork (NodeContainer nodes, std ::string routingMethod, uint32_t simulationTime);
  virtual ~MyNetwork();

  void BuildTopology (std::vector<int> adjacencyVec);
  void AddApplication (uint32_t src, uint32_t dst, double rate);
  FlowVec GetFlowVec();

private:

  ChannelMap m_channelMap;
  FlowVec m_flowVec;
  
  NodeContainer m_nodes;
  std::string m_routingMethod;
  uint32_t m_simulationTime;
  uint32_t m_applicationPort;
};
} // namespace ns3
#endif // MY_NETWORK_H
/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-29 15:54
 * @desc: RL概率路由计算的实现
 */

#include <utility>
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>
#include "ns3/assert.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/mpi-interface.h"
#include "rl-router-interface.h"
#include "rl-route-manager-impl.h"
#include "ipv4-rl-routing.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RLRouteManagerImpl");

// ---------------------------------------------------------------------------
//
// RLRoutingDB Implementation
//
// ---------------------------------------------------------------------------

RLRoutingDB::RLRoutingDB()
{
  NS_LOG_FUNCTION(this);
}

RLRoutingDB::~RLRoutingDB()
{
  NS_LOG_FUNCTION(this);
}

void RLRoutingDB::Initialize(int *adjacencyArray, NodeContainer nodes)
{
  NS_LOG_FUNCTION(this);
  // 计算节点数目
  uint32_t nodeNum = nodes.GetN();
  // 设置Matrix类型的AdjacencyMatrix
  SetAdjacencyMatrix(adjacencyArray, nodeNum);

  // 计算并设置Matrix格式的reachableMatrix
  CalcReachableMatrix(adjacencyArray, nodeNum);

  // 初始化out interface map
  InitNextNodeMatrix(nodes);

  // 初始化权重矩阵
  InitWeightMatrix(nodeNum);
}

void RLRoutingDB::SetWeightMatrix(double *weightArray, uint32_t nodeNum)
{
  NS_LOG_FUNCTION(this);
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    for (uint32_t nextHop = 0; nextHop < nodeNum; nextHop++)
    {
      double weight = weightArray[src * nodeNum + nextHop];
      Edge edge = Edge(src, nextHop);
      m_weightMatrix[edge] = weight;
    }
  }
}

double
RLRoutingDB::GetWeight(Edge edge)
{
  return m_weightMatrix[edge];
}

double
RLRoutingDB::GetWeight(NodeId src, NodeId nextHop)
{
  Edge edge = Edge(src, nextHop);
  return GetWeight(edge);
}

RLRoutingDB::NextNode
RLRoutingDB::GetNextNode(Edge edge)
{
  return m_nextNodeMatrix[edge];
}

RLRoutingDB::NextNode
RLRoutingDB::GetNextNode(NodeId src, NodeId nextHop)
{
  Edge edge = Edge(src, nextHop);
  return GetNextNode(edge);
}

int RLRoutingDB::IsAdjacency(Edge edge)
{
  return m_adjacencyMatrix[edge];
}

int RLRoutingDB::IsAdjacency(uint32_t src, uint32_t dst)
{
  Edge edge = Edge(src, dst);
  return IsAdjacency(edge);
}

int RLRoutingDB::IsReachable(Edge edge)
{
  return m_reachableMatrix[edge];
}

int RLRoutingDB::IsReachable(uint32_t src, uint32_t dst)
{
  Edge edge = Edge(src, dst);
  return IsReachable(edge);
}

int RLRoutingDB::IsValidPath(NodeId src, NodeId next, NodeId dst)
{
  int isAdjacency = IsAdjacency(src, next);
  int isReachable = IsReachable(next, dst);

  if (isAdjacency == -1)
  {
    return -2; // 下一跳不是邻接，返回-2
  }
  if (isAdjacency == 0)
  {
    return 0; // 下一跳是自身，返回0
  }

  if (isAdjacency == 1)
  {
    if (isReachable == -1)
    {
      return -1; // 下一跳到不了终点，返回-1
    }
    if (isReachable == 0)
    {
      return 1; // 下一跳直接到达终点，返回1
    }
    if (isReachable == 1)
    {
      return 2; // 经过下一跳能到达，返回2
    }
  }
  bool isValidPath = isAdjacency && isReachable;
  return isValidPath;
}

void RLRoutingDB::SetAdjacencyMatrix(int *adjacencyArray, uint32_t nodeNum)
{
  NS_LOG_FUNCTION(this);
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    for (uint32_t dst = 0; dst < nodeNum; dst++)
    {
      int adjacency = adjacencyArray[src * nodeNum + dst];
      Edge edge = Edge(src, dst);
      m_adjacencyMatrix[edge] = adjacency;
      NS_LOG_LOGIC("src: " << src << ", "
                           << "dst: " << dst << ", adjacency: " << adjacency);
    }
  }
}

void RLRoutingDB::CalcReachableMatrix(int *adjacencyArray, uint32_t nodeNum)
{
  NS_LOG_FUNCTION(this << " nodeNum: " << nodeNum);
  int reachableArray[nodeNum * nodeNum];
  memcpy(reachableArray, adjacencyArray,
         nodeNum * nodeNum * sizeof(int)); // copy adjacency to reachable
  // 计算reachableMatrix
  for (uint32_t next = 0; next < nodeNum; next++)
  { // 遍历转发节点
    for (uint32_t src = 0; src < nodeNum; src++)
    { // 遍历src
      for (uint32_t dst = 0; dst < nodeNum; dst++)
      { // 遍历dst
        if (reachableArray[src * nodeNum + dst] == -1)
        { // 只有不可达的才判断，减少运算量
          // 如果转发可达，则可达
          if (reachableArray[src * nodeNum + next] == 1 &&
              reachableArray[next * nodeNum + dst] == 1)
          {
            reachableArray[src * nodeNum + dst] = 1;
            NS_LOG_LOGIC(src << " to " << dst << " is reachable via " << next);
          }
        }
      }
    }
  }
  // 设置Matrix格式的可达矩阵
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    for (uint32_t dst = 0; dst < nodeNum; dst++)
    {
      int reachable = reachableArray[src * nodeNum + dst];
      Edge edge = Edge(src, dst);
      m_reachableMatrix[edge] = reachable;
      NS_LOG_LOGIC("src: " << src << ", "
                           << "dst: " << dst << ", reachable: " << reachable);
    }
  }
}

void RLRoutingDB::InitNextNodeMatrix(NodeContainer nodes)
{
  // 初始化oif映射关系
  NS_LOG_FUNCTION(this);

  uint32_t nodeNum = nodes.GetN();

  // 遍历所有src，查找到所有dst的out interface
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    Ptr<Node> srcNode = nodes.Get(src);          //  获取channel的src Node
    uint32_t deviceNum = srcNode->GetNDevices(); // 获取src Node 拥有的device数目
    for (uint32_t dst = 0; dst < nodeNum; dst++)
    {

      if (IsAdjacency(src, dst))
      { // 只有邻接矩阵上写了邻接的才考虑，否则即使有链路也视为不连通
        NS_LOG_LOGIC("Consider src " << src << "->"
                                     << "dst: " << dst);
      }
      else
      {
        NS_LOG_LOGIC("Ignore src " << src << "->"
                                   << "dst: " << dst);
        continue;
      }

      Ptr<Node> dstNode = nodes.Get(dst); //  获取channel的dst Node
      // 查找oif使得 src -- oif -->  dst
      uint32_t oifIndex = 0;
      // 遍历所有devices
      for (uint32_t deviceIndex = 0; deviceIndex < deviceNum; deviceIndex++)
      {
        Ptr<NetDevice> srcDevice = srcNode->GetDevice(deviceIndex);
        // 检查对端
        Ptr<Channel> channel = srcDevice->GetChannel();

        if (channel == 0)
        {
          NS_LOG_LOGIC("    deveiceIndex: " << deviceIndex << ", channel is "
                                            << "null");
          continue;
        }
        else
        {
          NS_LOG_LOGIC("    deveiceIndex: " << deviceIndex << ", channel is " << channel);
        }
        // 注意到是point-to-point信道，一定有且只有两个device
        // 需要检测的是 不是srcDevice 的那个Device
        Ptr<NetDevice> fitstDevice = channel->GetDevice(0);
        Ptr<NetDevice> targetDevice =
            fitstDevice == srcDevice ? channel->GetDevice(1) : channel->GetDevice(0);
        if (targetDevice->GetNode() == dstNode)
        {
          // 如果targetDevice是绑定在dstNode上的，则更新NextNode信息
          NS_LOG_LOGIC("  find target node is dst node");
          Ipv4Address remoteIp =
              targetDevice->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
          oifIndex = srcDevice->GetIfIndex();
          NextNode nextNode = NextNode(remoteIp, oifIndex);
          // 添加到map中
          Edge edge = Edge(src, dst);
          m_nextNodeMatrix[edge] = nextNode;
          NS_LOG_LOGIC("  src: " << src << ", "
                                 << "dst: " << dst << "; "
                                 << "oif: " << oifIndex << ", remoteIp: " << remoteIp);
          break;
        }
      }
      if (oifIndex == 0)
      {
        NS_LOG_LOGIC("  cannot find target node corresponding to dst node");
      }
    }
  }
}

void RLRoutingDB::InitWeightMatrix(uint32_t nodeNum)
{
  NS_LOG_FUNCTION(this);
  // 遍历所有发包节点
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    // 遍历所有下一跳
    for (uint32_t nextHop = 0; nextHop < nodeNum; nextHop++)
    {
      Edge edge = Edge(src, nextHop);
      // 如果两节点之间邻接，则将权重设为1；否则，权重设为0
      if (IsAdjacency(src, nextHop) == 1)
      {
        m_weightMatrix[edge] = 1;
      }
      else
      {
        m_weightMatrix[edge] = 0;
      }
    }
  }
}

// ---------------------------------------------------------------------------
//
// RLRouteManagerImpl Implementation
//
// ---------------------------------------------------------------------------

RLRouteManagerImpl::RLRouteManagerImpl()
{
  NS_LOG_FUNCTION(this);
  m_rldb = new RLRoutingDB();
}

RLRouteManagerImpl::~RLRouteManagerImpl()
{
  NS_LOG_FUNCTION(this);
  if (m_rldb)
  {
    delete m_rldb;
  }
}

void RLRouteManagerImpl::DeleteRoutes()
{
  NS_LOG_FUNCTION(this);
  for (NodeContainer::Iterator iter = m_nodes.Begin(); iter != m_nodes.End(); iter++)
  {
    Ptr<Node> node = *iter;
    Ptr<RLRouter> router = node->GetObject<RLRouter>();
    if (router == 0)
    {
      continue;
    }
    Ptr<Ipv4RLRouting> protocol = router->GetRoutingProtocol();
    uint32_t nRoutes = protocol->GetNRoutes();
    NS_LOG_LOGIC("Deleting " << protocol->GetNRoutes() << " routes from node " << node->GetId());
    // Each time we delete route 0, the route index shifts downward
    // We can delete all routes if we delete the route numbered 0
    // nRoutes times
    uint32_t rIndex;
    for (rIndex = 0; rIndex < nRoutes; rIndex++)
    {
      NS_LOG_LOGIC("Deleting global route " << rIndex << " from node " << node->GetId());
      protocol->RemoveRoute(0);
    }
    NS_LOG_LOGIC("Deleted " << rIndex << " global routes from node " << node->GetId());
  }
}

//
// 调用DB提供的接口即可完成数据库的构建
//
void RLRouteManagerImpl::BuildRLRoutingDatabase(int *adjacencyArray, NodeContainer nodes)
{
  NS_LOG_FUNCTION(this);
  m_nodes = nodes;
  m_rldb->Initialize(adjacencyArray, m_nodes);
}

// 设置权重矩阵
void RLRouteManagerImpl::SetWeightMatrix(double *weightArray)
{
  NS_LOG_FUNCTION(this);
  uint32_t nodeNum = m_nodes.GetN();
  m_rldb->SetWeightMatrix(weightArray, nodeNum);
}

// 遍历所有节点，依次作为src
// 对于src节点，遍历所有节点，作为nextHop；
// 对于确定src的确定nextHop，遍历所有节点作为dst；
// 如果 src->nextHop->dst是一个可行路径，则查找：
// 1. src->nextHop使用的出口interface
// 2. src->nextHop的权重
// 3. nextHop的所有IP
// 通过上述1. 2. 和所有的3. ，得到 (dstIp, nextHop, outIf, weight)
// 将其写入src的路由表
void RLRouteManagerImpl::CalculateRoutes()
{
  NS_LOG_FUNCTION(this);
  // 遍历所有节点
  uint32_t nodeNum = m_nodes.GetN();
  for (uint32_t src = 0; src < nodeNum; src++)
  {
    NS_LOG_LOGIC("src: " << src);
    Ptr<Node> srcNode = m_nodes.Get(src);
    for (uint32_t next = 0; next < nodeNum; next++)
    {
      Ptr<Node> nextNode = m_nodes.Get(next);
      for (uint32_t dst = 0; dst < nodeNum; dst++)
      {
        Ptr<Node> dstNode = m_nodes.Get(dst);

        // 考虑src->next->dst的路径，如果可行，则需要得到构成路由表的数据
        // 包括： outIf, nextHop, weight : 都是可以直接从DB中读取的
        // 以及： dstIp : 需要遍历dstNode
        // 基本逻辑： 能到达一个node，就能到达这个node上的所有ip
        NS_LOG_LOGIC("  Consider "
                     << src << "->" << next << "->" << dst << ""
                     << ", path valid type is: " << m_rldb->IsValidPath(src, next, dst));
        // 如果路径可行才继续

        if (m_rldb->IsValidPath(src, next, dst) > 0)
        {
          // 获取outIf, nextHop, weight信息
          Ipv4Address nextHop = m_rldb->GetNextNode(src, next).first;
          uint32_t outIf = m_rldb->GetNextNode(src, next).second;
          double weight = m_rldb->GetWeight(src, next);
          NS_LOG_LOGIC("  Path valid, outIf: " << outIf << ", nextHop: " << next
                                               << ", weight: " << weight);

          // 获取要设置的protocol
          Ptr<RLRouter> router = srcNode->GetObject<RLRouter>();
          if (router == 0)
          {
            continue;
          }
          Ptr<Ipv4RLRouting> gr = router->GetRoutingProtocol();

          // 遍历dstnode的所有interface，遍历所有interface的ip（通常只有一个）
          // 遍历所有dstIP，组成路由表元素并设置到src节点的protocol上
          uint32_t ifNum = dstNode->GetObject<Ipv4>()->GetNInterfaces();
          NS_LOG_LOGIC("dst node " << dst << " has " << ifNum << " interfaces");
          // if从1开始计数，if0表示127.0.0.1
          for (uint32_t ifIndex = 1; ifIndex < ifNum; ifIndex++)
          {
            // 获取这个if的ip数，遍历
            uint32_t ipNum = dstNode->GetObject<Ipv4>()->GetNAddresses(ifIndex);
            for (uint32_t ipIndex = 0; ipIndex < ipNum; ipIndex++)
            {
              Ipv4Address dstIp = dstNode->GetObject<Ipv4>()
                                      ->GetAddress(ifIndex, ipIndex)
                                      .GetLocal();
              // 设置路由表
              gr->AddHostRouteTo(dstIp, nextHop, outIf, weight);
              NS_LOG_LOGIC("    Node " << srcNode->GetId() << " adding host route to "
                                       << dstIp << " using next hop " << nextHop
                                       << " and outgoing interface " << outIf
                                       << " with weight " << weight);
            }
          }
        }
      }
    }
  }
  NS_LOG_INFO("Finished Route calculation");
}

RLRoutingDB *
RLRouteManagerImpl::DebugGetRLDB()
{
  return m_rldb;
}
} // namespace ns3

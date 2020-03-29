/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-29 15:57
 * @desc: 测试RLRouteManagerImpl是否按照预期工作
 */

// 测试RLRouteManagerImpl是否按照预期工作。
//
// RLDBInitiaizeTestCase 介绍
//
//      网络拓扑:             n0---->n1---->n2---->n3
//      邻接矩阵使用:         [ 0,  1, -1, -1,
//                           -1,  0,  1, -1,
//                           -1, -1,  0,  1,
//                           -1, -1, -1,  0 ]
//
//      a. 测试邻接矩阵的设置:  遍历src->dst:
//                            1. src==dst的时候adjecency==0
//                            2. dst==src+1 && src < 3 的时候adjacency==1
//                            3. elsewise adjacency==-1
//
//      b. 测试可达矩阵的计算:  遍历src->dst:
//                            1. src<dst: reachable == 1
//                            2. src==dst: reachable == 0
//                            3. src>dst: reachable == -1
//      可达矩阵应该是:        [ 0,  1,  1,  1,
//                           -1,  0,  1,  1,
//                           -1, -1,  0,  1,
//                           -1, -1, -1,  0 ]
//
//      c. 测试可行路径的计算:  对几个特殊路径进行检测:
//                            1. 0->1->3: valid ==  2(转发后可达)
//                            2. 1->2->2: valid ==  1(下一跳就是目的地)
//                            3. 0->0->1: valid ==  0(下一跳是自身)
//                            4. 2->3->0: valid == -1(到达下一跳之后无法到达目的地)
//                            5. 0->2->3: valid == -2(下一跳不可达)
//
//      d. 测试初始化权重矩阵:  遍历(src, dst), 核验:
//                            weight of (src, dst) == adjacency of (src, dst)
// 
//  RLRouteManagerImplTestSuite 介绍:
// 
//      网络拓扑:             
//                      ---------------->n1---------------->
//          (10.0.1.1) |     (10.0.1.2)       (10.0.3.1)   | (10.0.3.2)
//         (node0 if1) |    (node1 if1)       (node1 if2)  | (node3 if1)
//                  n0 |                                   | n3  
//          (10.0.2.1) |     (10.0.2.2)       (10.0.4.1)   | (10.0.4.2)
//         (node0 if2) |    (node2 if1)       (node2 if2)  | (node3 if2)
//                      ---------------->n2---------------->
// 
//      相应的邻接矩阵为:      [ 0,  1,  1, -1,
//                           -1,  0, -1,  1,
//                           -1, -1,  0,  1,
//                           -1, -1, -1,  0 ]
// 
//      使用的权重矩阵为:      [ 0, 0.3, 0.7,  0,
//                            0,   0,   0,  1,
//                            0,   0,   0,  1,
//                            0,   0,   0,  0 ]
//      a. 测试权重矩阵:      检查特殊值
//                            1. weight of (0, 1) == 0.3
//                            2. weight of (0, 2) == 0.7
//                            1. weight of (1, 0) == 0
//                        
// 
//      应该形成的路由表为:   
//                          path         dstIp      nextHop     outIf   weight
//                          n0--->n1 :  10.0.1.2   10.0.1.2       1      0.3
//                          n0--->n1 :  10.0.3.1   10.0.1.2       1      0.3
//                          n0--->n2 :  10.0.2.2   10.0.2.2       2      0.7
//                          n0--->n2 :  10.0.4.1   10.0.2.2       2      0.7
//                    n0--->n1--->n3 :  10.0.3.2   10.0.1.2       1      0.3
//                    n0--->n1--->n3 :  10.0.4.2   10.0.1.2       1      0.3
//                    n0--->n2--->n3 :  10.0.3.2   10.0.2.2       2      0.7
//                    n0--->n2--->n3 :  10.0.4.2   10.0.2.2       2      0.7
//                          n1--->n3 :  10.0.3.2   10.0.3.2       2       1
//                          n1--->n3 :  10.0.4.2   10.0.3.2       2       1
//                          n2--->n3 :  10.0.3.2   10.0.3.2       2       1
//                          n2--->n3 :  10.0.4.2   10.0.4.2       2       1
//      b. 测试路由表:        检查所有节点路由表
//                            1. 节点路由表数目要对应
//                            2. 所有路由表应该在对应节点能找到
//                             
#include "ns3/core-module.h"
#include "ns3/test.h"
#include "ns3/rl-route-manager-impl.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RLRouteManagerImplTestSuite");

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief 用于检测DB Init 是否正常
 */
class RLDBInitiaizeTestCase : public TestCase
{
public:
  RLDBInitiaizeTestCase ();
  virtual void DoRun (void);

private:
  NodeContainer m_nodes;
};

RLDBInitiaizeTestCase::RLDBInitiaizeTestCase () : TestCase ("RLDBInitiaizeTestCase")
{
  // 创建四个nodes用于测试
  NodeContainer nodes;
  nodes.Create (4);
  m_nodes = nodes;
}
void
RLDBInitiaizeTestCase::DoRun (void)
{
  // 创建adjacencyArray用于测试
  int m_adjacencyArray[16] = {0, 1, -1, -1, -1, 0, 1, -1, -1, -1, 0, 1, -1, -1, -1, 0};
  // 配置拓扑结构
  uint32_t nodeNum = m_nodes.GetN ();
  // 为node配置协议栈
  InternetStackHelper stack;
  Ipv4ListRoutingHelper listRouting;
  Ipv4RLRoutingHelper rlRouting;
  listRouting.Add (rlRouting, -10);
  stack.SetRoutingHelper (listRouting);
  stack.Install (m_nodes);

  // 初步的测试中不区分信道素质，都设置为同样的信道
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); // 数据速率5Mbps
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms")); // 传输时延2ms
  Ipv4AddressHelper address;

  uint32_t ipBase = 0;
  // 遍历节点对，创建信道
  for (uint32_t sindex = 0; sindex < nodeNum; sindex++)
    {
      for (uint32_t dindex = 0; dindex < nodeNum; dindex++)
        {
          // 如果距离矩阵值无效则跳过。对下三角区域，直接判断是否无效；对上三角区域，要判断是否会重复
          if (m_adjacencyArray[dindex * nodeNum + sindex] != 1)
            continue;
          if (sindex > dindex && (m_adjacencyArray[sindex * nodeNum + dindex] == 1))
            continue;

          // 配置信道并获得返回的device
          NetDeviceContainer tempDevices =
              pointToPoint.Install (m_nodes.Get (sindex), m_nodes.Get (dindex));
          // 为信道两端的device配置ip成为interface
          char strBase[10];
          sprintf (strBase, "10.%d.%d.0", ipBase / 256 + 1, ipBase % 256 + 1);
          address.SetBase (strBase, "255.255.255.0");
          Ipv4InterfaceContainer tempInter = address.Assign (tempDevices);
          ipBase += 1;
        }
    }

  // 调用impl方法初始化数据库
  RLRouteManagerImpl manager;
  manager.BuildRLRoutingDatabase (m_adjacencyArray, m_nodes);
  RLRoutingDB *rldb = manager.DebugGetRLDB ();

  // 测试邻接矩阵的设置
  for (uint32_t src = 0; src < nodeNum; src++)
    {
      for (uint32_t dst = 0; dst < nodeNum; dst++)
        {
          int targetAdjacency = 0;
          if (src == dst)
            {
              targetAdjacency = 0;
            }
          else if (src == dst - 1 && src < 3)
            {
              targetAdjacency = 1;
            }
          else
            {
              targetAdjacency = -1;
            }
          NS_TEST_ASSERT_MSG_EQ (rldb->IsAdjacency (src, dst), targetAdjacency,
                                 "Error: 邻接关系设置错误");
        }
    }

  // 测试可达矩阵的计算
  for (uint32_t src = 0; src < nodeNum; src++)
    {
      for (uint32_t dst = 0; dst < nodeNum; dst++)
        {
          int targetReachable = 0;
          if (src == dst)
            {
              targetReachable = 0;
            }
          else if (src < dst)
            {
              targetReachable = 1;
            }
          else
            {
              targetReachable = -1;
            }
          NS_TEST_ASSERT_MSG_EQ (rldb->IsReachable (src, dst), targetReachable,
                                 "Error(" << src << ", " << dst << "): 可达矩阵计算错误");
        }
    }

  // 测试可行路径的计算
  NS_TEST_ASSERT_MSG_EQ (rldb->IsValidPath (0, 1, 3), 2,
                         "Error: 可行路径的计算错误-case 2(转发后可达)");
  NS_TEST_ASSERT_MSG_EQ (rldb->IsValidPath (1, 2, 2), 1,
                         "Error: 可行路径的计算错误-case 1(下一跳就是目的地)");
  NS_TEST_ASSERT_MSG_EQ (rldb->IsValidPath (0, 0, 1), 0,
                         "Error: 可行路径的计算错误-case 0(下一跳是自身)");
  NS_TEST_ASSERT_MSG_EQ (rldb->IsValidPath (2, 3, 0), -1,
                         "Error: 可行路径的计算错误-case -2(到达下一跳之后无法到达目的地)");
  NS_TEST_ASSERT_MSG_EQ (rldb->IsValidPath (0, 2, 3), -2,
                         "Error: 可行路径的计算错误-case -1(下一跳不可达)");

  // 测试权重矩阵的初始化
  for (uint32_t src = 0; src < nodeNum; src++)
    {
      for (uint32_t dst = 0; dst < nodeNum; dst++)
        {
          if (rldb->IsAdjacency (src, dst) == 1)
            {
              NS_TEST_ASSERT_MSG_EQ (rldb->GetWeight (src, dst), 1, "Error: 权重矩阵初始化错误");
            }
          else
            {
              NS_TEST_ASSERT_MSG_EQ (rldb->GetWeight (src, dst), 0, "Error: 权重矩阵初始化错误");
            }
        }
    }
}

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief 用于检测路由表计算是否正常
 */
class CalculateRoutesTestCase : public TestCase
{
public:
  CalculateRoutesTestCase ();
  virtual void DoRun (void);
private:
  NodeContainer m_nodes;
};

CalculateRoutesTestCase::CalculateRoutesTestCase () : TestCase ("CalculateRoutesTestCase")
{
  // 创建四个nodes用于测试
  NodeContainer nodes;
  nodes.Create (4);
  m_nodes = nodes;
}
void
CalculateRoutesTestCase::DoRun (void)
{
  // 创建adjacencyArray用于测试
  int m_adjacencyArray[16] = {0, 1, 1, -1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0};
  // 创建weightArray用于测试
  double m_weightArray[16] = {0, 0.3, 0.7, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};
  // 配置拓扑结构
  // 为node配置协议栈
  InternetStackHelper stack;
  Ipv4ListRoutingHelper listRouting;
  Ipv4RLRoutingHelper rlRouting;
  listRouting.Add (rlRouting, -10);
  stack.SetRoutingHelper (listRouting);
  stack.Install (m_nodes);

  // 初步的测试中不区分信道素质，都设置为同样的信道
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); // 数据速率5Mbps
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms")); // 传输时延2ms
  Ipv4AddressHelper address;

  // 创建信道
  NetDeviceContainer tempDevices;
  tempDevices = pointToPoint.Install (m_nodes.Get (0), m_nodes.Get (1));
  address.SetBase ("10.0.1.0", "255.255.255.0");
  address.Assign (tempDevices);
  tempDevices = pointToPoint.Install (m_nodes.Get (0), m_nodes.Get (2));
  address.SetBase ("10.0.2.0", "255.255.255.0");
  address.Assign (tempDevices);
  tempDevices = pointToPoint.Install (m_nodes.Get (1), m_nodes.Get (3));
  address.SetBase ("10.0.3.0", "255.255.255.0");
  address.Assign (tempDevices);
  tempDevices = pointToPoint.Install (m_nodes.Get (2), m_nodes.Get (3));
  address.SetBase ("10.0.4.0", "255.255.255.0");
  address.Assign (tempDevices);
  

  // 调用impl方法初始化数据库
  RLRouteManagerImpl manager;
  manager.BuildRLRoutingDatabase (m_adjacencyArray, m_nodes);
  // 设置权重矩阵
  manager.SetWeightMatrix(m_weightArray);
  // 计算路由表
  manager.CalculateRoutes();
  RLRoutingDB *rldb = manager.DebugGetRLDB ();

  // 检查权重矩阵
  NS_TEST_ASSERT_MSG_EQ(rldb->GetWeight(0, 1), 0.3, "Error: 权重设置错误");
  NS_TEST_ASSERT_MSG_EQ(rldb->GetWeight(0, 2), 0.7, "Error: 权重设置错误");
  NS_TEST_ASSERT_MSG_EQ(rldb->GetWeight(1, 0), 0, "Error: 权重设置错误");

  // 测试各个节点路由表是否被正确设置
  Ptr<RLRouter> router;
  Ptr<Ipv4RLRouting> protocol;
  Ipv4RLRouting::HostRoutesCI iter;
  // node 0
  router = m_nodes.Get(0)->GetObject<RLRouter> ();
  NS_TEST_ASSERT_MSG_NE(router, 0, "Error: 节点对应的路由器为0");
  protocol = router->GetRoutingProtocol ();
  NS_TEST_ASSERT_MSG_EQ(protocol->GetNRoutes(), 8, "Error: 路由表行数错误");

  //                          n0--->n1 :  10.0.1.2   10.0.1.2       1      0.3
  //                          n0--->n1 :  10.0.3.1   10.0.1.2       1      0.3
  //                          n0--->n2 :  10.0.2.2   10.0.2.2       2      0.7
  //                          n0--->n2 :  10.0.4.1   10.0.2.2       2      0.7
  //                    n0--->n1--->n3 :  10.0.3.2   10.0.1.2       1      0.3
  //                    n0--->n1--->n3 :  10.0.4.2   10.0.1.2       1      0.3
  //                    n0--->n2--->n3 :  10.0.3.2   10.0.2.2       2      0.7
  //                    n0--->n2--->n3 :  10.0.4.2   10.0.2.2       2      0.7
  // 遍历路由表项，按照上述顺序给予编号；找到编号对应的路由，则编号对应的计数+1
  // 最终需要所有编号的计数都是1，否则就有问题；有不在列表里的路由表项也有问题
  uint32_t routingCountArray[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for(uint32_t index=0; index < 8; index ++){
    iter = protocol->GetRoute(index);
    if(iter->first->GetDest() == "10.0.1.2" && iter->first->GetGateway() == "10.0.1.2" 
      && iter->first->GetInterface() == 1 && iter->second == 0.3){
        routingCountArray[0] += 1;
      }
    else if(iter->first->GetDest() == "10.0.3.1" && iter->first->GetGateway() == "10.0.1.2" 
    && iter->first->GetInterface() == 1 && iter->second == 0.3){
        routingCountArray[1] += 1;
      }
    else if(iter->first->GetDest() == "10.0.2.2" && iter->first->GetGateway() == "10.0.2.2" 
    && iter->first->GetInterface() == 2 && iter->second == 0.7){
        routingCountArray[2] += 1;
      }
    else if(iter->first->GetDest() == "10.0.4.1" && iter->first->GetGateway() == "10.0.2.2" 
    && iter->first->GetInterface() == 2 && iter->second == 0.7){
        routingCountArray[3] += 1;
      }
    else if(iter->first->GetDest() == "10.0.3.2" && iter->first->GetGateway() == "10.0.1.2" 
    && iter->first->GetInterface() == 1 && iter->second == 0.3){
        routingCountArray[4] += 1;
      }
    else if(iter->first->GetDest() == "10.0.4.2" && iter->first->GetGateway() == "10.0.1.2" 
    && iter->first->GetInterface() == 1 && iter->second == 0.3){
        routingCountArray[5] += 1;
      }
    else if(iter->first->GetDest() == "10.0.3.2" && iter->first->GetGateway() == "10.0.2.2" 
    && iter->first->GetInterface() == 2 && iter->second == 0.7){
        routingCountArray[6] += 1;
      }
    else if(iter->first->GetDest() == "10.0.4.2" && iter->first->GetGateway() == "10.0.2.2" 
    && iter->first->GetInterface() == 2 && iter->second == 0.7){
        routingCountArray[7] += 1;
      }
    else{
      NS_TEST_ASSERT_MSG_EQ (true, false, "Error: 出现不在列表中的路由表项");
    }
  }
  // 上述只是检测是不是有不在表里的路由表项，下面还要检测是不是每一条表项都各为一个
  for(uint32_t i = 0; i < 8; i++){
    NS_TEST_ASSERT_MSG_EQ (routingCountArray[i], 1, "Error: 路由表项缺失");
  }
}

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief RL Route Manager TestSuite
 */
class RLRouteManagerImplTestSuite : public TestSuite
{
public:
  RLRouteManagerImplTestSuite ();

private:
};

RLRouteManagerImplTestSuite::RLRouteManagerImplTestSuite ()
    : TestSuite ("rl-route-manager-impl", UNIT)
{
  AddTestCase (new RLDBInitiaizeTestCase (), TestCase::QUICK);
  AddTestCase (new CalculateRoutesTestCase (), TestCase::QUICK);
}

static RLRouteManagerImplTestSuite
    g_rlRoutingManagerImplTestSuite; //!< Static variable for test initialization

/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-02 17:28
 * @FilePath: /simulator/udp-tm/mynetwork.cc
 */

#include "mynetwork.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyNetwork");

MyNetwork::MyNetwork ()
{
  m_applicationPort = 615;
}

MyNetwork::MyNetwork (NodeContainer nodes, std ::string routingMethod, uint32_t simulationTime)
{
  m_nodes = nodes;
  m_routingMethod = routingMethod;
  m_simulationTime = simulationTime;
  m_applicationPort = 615;
}

MyNetwork::~MyNetwork ()
{
  NS_LOG_FUNCTION (this);
}

void
MyNetwork::BuildTopology (std::vector<int> adjacencyVec)
{
  // 将顺序改为输入距离矩阵后自动创建信道等
  uint32_t nodeNum = m_nodes.GetN ();
  // 创建一个vector用于记录该node的device index用到哪了。初始化为0。
  std::vector<uint32_t> nodeDeviceVec = std::vector<uint32_t> (nodeNum, 0);

  // 为nodes配置协议栈
  InternetStackHelper stack;
  Ipv4ListRoutingHelper listRouting;
  // 根据配置的路由规则配置路由规则
  if (m_routingMethod == "rl")
    {
      Ipv4RLRoutingHelper rlRouting;
      listRouting.Add (rlRouting, -10);
    }
  else
    {
      Ipv4GlobalRoutingHelper globalRouting;
      listRouting.Add (globalRouting, -10);
    }
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
          if (adjacencyVec[sindex * nodeNum + dindex] != 1)
            {
              continue;
            }
          if (sindex > dindex && (adjacencyVec[dindex * nodeNum + sindex] == 1))
            {
              continue;
            }

          // 配置信道并获得返回的device，顺序和传入的node顺序一致
          NetDeviceContainer tempDevices =
              pointToPoint.Install (m_nodes.Get (sindex), m_nodes.Get (dindex));
          NS_LOG_DEBUG ("将P2P信道应用到" << sindex << ", " << dindex);
          // 为信道两端的device配置ip成为interface
          char strBase[10];
          sprintf (strBase, "10.%d.%d.0", ipBase / 256 + 1, ipBase % 256 + 1);
          address.SetBase (strBase, "255.255.255.0");
          Ipv4InterfaceContainer tempInter = address.Assign (tempDevices);
          ipBase += 1;

          // 记录<node, interface>与<node, node>的关系
          m_channelMap[NodeInterfacePair (sindex, tempDevices.Get (0)->GetIfIndex ())] =
              NodePair (sindex, dindex);
          m_channelMap[NodeInterfacePair (dindex, tempDevices.Get (1)->GetIfIndex ())] =
              NodePair (dindex, sindex);
        }
    }

  //   放弃pcap
  // pointToPoint.EnablePcapAll ("router");
}

void
MyNetwork::AddApplication (uint32_t src, uint32_t dst, double rate)
{
  // 部分固定配置
  NS_LOG_DEBUG ("src: " << src << ", dst: " << dst << ", rate: " << rate);
  Time serverStartTime = Seconds (1.0);
  Time clientStartTime = Seconds (2.0);
  // 计算要发送的包数目和时间间隔（ms）
  // rate的单位是Mbps。1s应该发出1024*1024*ratebit；而一个包是1024 * 8 bit，所以1s应该发128*rate个包
  // 所以interval就是1000/(128*rate) = 125/(16*rate)
  uint64_t microInterval = 1000000 / (128 * rate);
  NS_LOG_DEBUG ("interval, " << microInterval);
  uint64_t packetsNum = m_simulationTime * 1000000 / microInterval;
  // 设置仿真时间
  Time intervalTime = MicroSeconds (microInterval);
  // server 配置在node dst 上
  UdpServerHelper udpServer (m_applicationPort);
  ApplicationContainer serverApps = udpServer.Install (m_nodes.Get (dst));
  serverApps.Start (serverStartTime);
  serverApps.Stop (Seconds (m_simulationTime + 5.0));

  // client配置在node src上，目标是node dst 的interface1 （注意interface是从1开始计数)
  UdpClientHelper udpClient (m_nodes.Get (dst)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (),
                             m_applicationPort);
  udpClient.SetAttribute ("MaxPackets", UintegerValue (packetsNum));
  udpClient.SetAttribute ("Interval", TimeValue (intervalTime));
  udpClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = udpClient.Install (m_nodes.Get (src));
  clientApps.Start (clientStartTime);
  clientApps.Stop (Seconds (m_simulationTime + 5.0));

  // port自增
  m_applicationPort += 1;
  // 将这组Flow放进Vec中
  m_flowVec.push_back (NodePair (src, dst));
}

MyNetwork::FlowVec
MyNetwork::GetFlowVec ()
{
  return m_flowVec;
}
} // namespace ns3
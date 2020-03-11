/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-rl-routing-helper.h"
#include "../rapidjson/document.h"

using namespace ns3;
using namespace rapidjson;
NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

typedef std::pair<uint32_t, uint32_t> Edge; //!< 两个node（由index确定）可以确定一条有向边
typedef std::pair<uint32_t, uint32_t> Flow; //!< 两个node（由index确定）可以确定一条Flow
typedef std::pair<uint32_t, uint32_t>
    ChannelId; //!< node的index和其上interface的index共同组成channelId
typedef std::map<ChannelId, Edge> ChannelMap; //!< 两种确定边的方式构成Map
typedef std::vector<Flow> FlowVec; //!< 记录FlowId和对应的node的关系的Vector

ChannelMap channelMap;
FlowVec flowVec;
std::vector<int> adjacencyMatrixVec = std::vector<int> ();

void
GetAvgDelay (Ptr<FlowMonitor> fptr)
{

  FlowMonitor::FlowStatsContainer flowStatsContainer = fptr->GetFlowStats ();
  std::map<FlowId, FlowMonitor::FlowStats>::iterator it;
  int64_t t = 0;
  uint32_t pnum = 0;
  for (it = flowStatsContainer.begin (); it != flowStatsContainer.end (); it++)
    {
      t += it->second.delaySum.GetNanoSeconds ();
      pnum += it->second.rxPackets;
      NS_LOG_DEBUG ("flow delaySum = " << it->second.delaySum.GetNanoSeconds ()
                                       << "; rxPackets = " << it->second.rxPackets);
    }
  NS_LOG_DEBUG ("average delay: " << t / pnum << "ns");
  NS_LOG_DEBUG ("average delay: " << t / pnum / 1000000 << "ms");
  fptr->SerializeToXmlFile ("myanal.xml", true, true);
}

void
GetForwardMatrix (Ptr<FlowMonitor> fptr, uint32_t nodeNum)
{
  NodeContainer m_nodes = NodeContainer::GetGlobal ();
  std::vector<uint32_t> forwardMatrix = std::vector<uint32_t> (nodeNum * nodeNum, 0);
  FlowMonitor::FlowProbeContainer flowProbeContainer = fptr->GetAllProbes ();
  for (uint32_t index = 0; index < flowProbeContainer.size (); index++)
    {
      FlowProbe::RLStats rlstats = flowProbeContainer[index]->GetRLStats ();
      for (auto sit = rlstats.begin (); sit != rlstats.end (); sit++)
        {
          NS_LOG_DEBUG ("FlowId: " << sit->first.flowId << "; NodeId: " << sit->first.nodeId
                                   << "; IfId: " << sit->first.interface << ". -> packtes: "
                                   << sit->second.packets);
          // ! 注意RLStats中的NodeId确实是ID而不是Index
          // Id是全局唯一的NodeList的index，同时，如果使用NodeContainer::GetGlobal ()
          // 取得的container实际上和NodeList是等价的，顺序也是一致的。
          // 即： 只要没有创建冗余的Node，这里的Id和nodes的Index是等价的
          uint32_t src = sit->first.nodeId;
          uint32_t interface = sit->first.interface;
          // 通过src 和 interface 可以获取另一段的Node
          Ptr<NetDevice> device= 0;
          for(uint32_t dindex = 0; dindex < m_nodes.Get(src)->GetNDevices(); dindex++){
            device = m_nodes.Get(src)->GetDevice(dindex);
            if(device->GetIfIndex() == interface){
              break;
            }
            device = 0;
          }
          if(device == 0){
            NS_LOG_UNCOND("未找到device");
          }
          // 获取与其相连的channel，查找对端
          Ptr<Channel> channel = device->GetChannel();
          if(channel == 0){
            NS_LOG_UNCOND("未找到channel");
          }
          // 因为是point-to-point信道，所以一定是两个device，一个是自己，另一个就是对端
          uint32_t remoteIndex = channel->GetDevice(0) == device ? 1 : 0;
          // 此时device已经没用了，可以用来记录对端device
          device = channel->GetDevice(remoteIndex);
          // 通过device查找对端Node
          uint32_t dst = device->GetNode()->GetId();
          Edge e = channelMap[ChannelId (sit->first.nodeId, sit->first.interface)];
          forwardMatrix[e.first * nodeNum + e.second] += sit->second.packets;
          NS_LOG_UNCOND("search: src="<<src<<", dst="<<dst<<"; store: src="<<e.first
          <<", dst="<<e.second);
        }
    }
  for (uint32_t i = 0; i < nodeNum; i++)
    {
      for (uint32_t j = 0; j < nodeNum; j++)
        {
          NS_LOG_DEBUG ("node " << i << " and node " << j << " have "
                                << forwardMatrix[i * nodeNum + j] << " packets.");
        }
    }
}

void
TopologyInit (NodeContainer nodes, std::vector<int> adjacencyMatrixVec)
{
  // 将顺序改为输入距离矩阵后自动创建信道等
  uint32_t nodeNum = nodes.GetN ();
  // 创建一个vector用于记录该node的device index用到哪了。初始化为0。
  std::vector<uint32_t> nodeDeviceVec = std::vector<uint32_t> (nodeNum, 0);

  // 为node配置协议栈
  InternetStackHelper stack;
  Ipv4RLRoutingHelper rlRouting;
  Ipv4ListRoutingHelper listRouting;
  listRouting.Add (rlRouting, -10);
  stack.SetRoutingHelper (listRouting);
  stack.Install (nodes);

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
          if (adjacencyMatrixVec[dindex * nodeNum + sindex] != 1)
            {
              continue;
            }
          if (sindex > dindex && (adjacencyMatrixVec[sindex * nodeNum + dindex] == 1))
            {
              continue;
            }

          // 配置信道并获得返回的device
          NetDeviceContainer tempDevices =
              pointToPoint.Install (nodes.Get (sindex), nodes.Get (dindex));
          NS_LOG_DEBUG ("将P2P信道应用到" << sindex << ", " << dindex);
          // 为信道两端的device配置ip成为interface
          char strBase[10];
          sprintf (strBase, "10.%d.%d.0", ipBase / 256 + 1, ipBase % 256 + 1);
          address.SetBase (strBase, "255.255.255.0");
          Ipv4InterfaceContainer tempInter = address.Assign (tempDevices);
          ipBase += 1;

          // 记录<node, interface>与<node, node>的关系
          // 利用了一个前提: device从0计数，interface从1计数
          channelMap[ChannelId (sindex, nodes.Get (sindex)->GetNDevices () - 1)] =
              Edge (sindex, dindex);
          channelMap[ChannelId (dindex, nodes.Get (dindex)->GetNDevices () - 1)] =
              Edge (dindex, sindex);
        }
    }

  //   开启所有pcap
  // pointToPoint.EnablePcapAll ("router");
}

void
ApplicationInit (uint32_t src, uint32_t dst, double rate, uint32_t applicationPort,
                 NodeContainer nodes, uint32_t simulationTime)
{
  // 部分固定配置
  NS_LOG_DEBUG("src: "<<src<<", dst: "<<dst<<", rate: "<<rate);
  Time serverStartTime = Seconds(1.0);
  Time clientStartTime = Seconds(2.0);
  // 计算要发送的包数目和时间间隔（ms）
  // rate的单位是Mbps。1s应该发出1024*1024*ratebit；而一个包是1024 * 8 bit，所以1s应该发128*rate个包
  // 所以interval就是1000/(128*rate) = 125/(16*rate)
  // uint64_t microInterval = 1000000 / (128 * rate);
  // NS_LOG_DEBUG("interval, "<<microInterval);
  // uint64_t packetsNum = simulationTime * 1000000 / microInterval;
  // 设置仿真时间
  // Time intervalTime = MicroSeconds (microInterval);
  // 计算bps为单位的速率
  uint64_t dataRate = (uint64_t)rate * 1000000;
  // server 配置在node dst 上
  ApplicationContainer sinkApp;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), applicationPort));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkApp.Add(sinkHelper.Install(nodes.Get(src)));
  sinkApp.Start (serverStartTime);
  sinkApp.Stop (Seconds(simulationTime + 5.0));


  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper.SetAttribute ("PacketSize", UintegerValue (1024));
  clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  // client配置在node src上，目标是node dst 的interface1 （注意interface是从1开始计数)
  ApplicationContainer clientApps;
  AddressValue remoteAddress(InetSocketAddress (nodes.Get(src)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), applicationPort));
  clientHelper.SetAttribute("Remote",remoteAddress);
  clientApps.Add(clientHelper.Install(nodes.Get(dst)));
  // 发包这件事要单独配置
  clientApps.Start (clientStartTime);
  clientApps.Stop (Seconds(simulationTime + 5.0));

  // 将这组Flow放进Vec中
  flowVec.push_back (Flow (src, dst));
}

int
main (int argc, char *argv[])
{
  // 使用cmd的时候要通过Config设置属性，要让Config在cmd之前
  Config::SetDefault ("ns3::Ipv4RLRouting::RandomEcmpRouting", StringValue ("true"));
  Time::SetResolution (Time::NS); // 设置仿真时间分辨率

  // 设置log级别
  LogComponentEnable ("FirstScriptExample", LOG_LEVEL_INFO);
  //  只有需要利用输出信息调试的时候才开启下方输出
  //  ns3::LogLevel level = (enum ns3::LogLevel)(LOG_LEVEL_FUNCTION|LOG_PREFIX_FUNC);
  //  LogComponentEnable("Ipv4RLRouting", level);
  //  LogComponentEnable("RLRouteManagerImpl", level);
  //  ns3::LogLevel nlevel = (enum ns3::LogLevel)(LOG_LEVEL_LOGIC|LOG_PREFIX_FUNC);
  //  LogComponentEnable("RLRouteManagerImpl", nlevel);
  // 指定仿真参数，包括随机种子、仿真时间、获取state的间隔、仿真端口。
  uint32_t simSeed = 1;
  double simulationTime = 5 * 2;
  double envStepTime = 5;

  // 邻接矩阵在外部声明
  std::string adjacencyMatrixStr = "[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]";
  std::string trafficMatrixStr = "[{/src/:0,/rate/:4.7,/dst/:3}]";
  CommandLine cmd;
  // required parameters for OpenGym interface
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 5", simSeed);
  // optional parameters
  cmd.AddValue ("simTime", "Simulation time in seconds. Default: 10s", simulationTime);
  cmd.AddValue ("forwardMatrix",
                "Assigned O-D TM, json type. Default: [{/src/:0,/rate/:3,/dst/:3}]",
                trafficMatrixStr);
  cmd.AddValue ("adjacencyMatrix",
                "Adjacency Matrix, json type, using -1 replace infinity. Defatult: "
                "[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]",
                adjacencyMatrixStr);
  cmd.Parse (argc, argv);
  replace (trafficMatrixStr.begin (), trafficMatrixStr.end (), '/', '"');

  NS_LOG_UNCOND ("Ns3Env parameters:");
  NS_LOG_UNCOND ("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND ("--forwardMatrix: " << trafficMatrixStr);
  NS_LOG_UNCOND ("--adjacencyMatrix: " << adjacencyMatrixStr);
  NS_LOG_UNCOND ("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND ("--seed: " << simSeed);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);

  Document root;
  // 初始化拓扑结构参数
  root.Parse (adjacencyMatrixStr.c_str ());
  for (auto &item : root.GetArray ())
    {
      auto adjacency = item.GetInt ();
      adjacencyMatrixVec.push_back (adjacency);
      NS_LOG_UNCOND ("push a adjacency relation: " << adjacency);
    }
  uint32_t nodeNum = sqrt (adjacencyMatrixVec.size ());
  int adjacencyArray[nodeNum * nodeNum];
  std::copy (adjacencyMatrixVec.begin (), adjacencyMatrixVec.end (), adjacencyArray);

  // 根据节点数目创建节点备用
  NodeContainer nodes;
  nodes.Create (nodeNum);

  // 初始化拓扑结构
  TopologyInit (nodes, adjacencyMatrixVec);

  // 配置流分析器
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.RLInstallAll ();
  Ptr<Ipv4FlowClassifier> flowClassifier =
      DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());

  // 确定停止时间
  Time endTime = Seconds (simulationTime + 5.0);

  // 根据业务TM配置应用层
  root.Parse (trafficMatrixStr.c_str ());
  for (auto &item : root.GetArray ())
    {
      auto traffic = item.GetObject ();
      static uint32_t applicationPort = 615;
      ApplicationInit (traffic["src"].GetInt (), traffic["dst"].GetInt (),
                       traffic["rate"].GetDouble (), applicationPort, nodes, simulationTime);
      applicationPort += 1;
    }

  double weightArray[16] = {0, 0.1, 0.1, 0.8, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};
  Ipv4RLRoutingHelper::InitializeRouteDatabase (adjacencyArray, nodes);
  Ipv4RLRoutingHelper::ComputeRoutingTables (weightArray);

  Simulator::Schedule (endTime, &GetAvgDelay, flowMonitor);
  Simulator::Schedule (endTime, &GetForwardMatrix, flowMonitor, nodeNum);
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime + 5));
  Simulator::Run ();

  NS_LOG_UNCOND ("Simulation stop");
  Simulator::Destroy ();

  return 0;
}
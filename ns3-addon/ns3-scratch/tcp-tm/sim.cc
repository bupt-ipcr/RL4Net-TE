/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#include <math.h>
#include <algorithm>
#include "myenv.h"
#include "mynetwork.h"
#include "ns3/log.h"
#include "../rapidjson/document.h"
#include "ns3/openenv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-rl-routing-helper.h"

using namespace ns3;
using namespace rapidjson;
NS_LOG_COMPONENT_DEFINE ("UdpTMSim");

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("OpenEnv", LOG_LEVEL_DEBUG);
  //  只有需要利用输出信息调试的时候才开启下方输出
  ns3::LogLevel level = (enum ns3::LogLevel) (LOG_LEVEL_LOGIC | LOG_PREFIX_FUNC);
  LogComponentEnable ("UdpTMSim", level);
  // LogComponentEnable ("MyOpenEnv", level);
  // LogComponentEnable ("Ipv4RLRouting", level);
  // LogComponentEnable ("RLRouteManagerImpl", level);

  // 仿真默认参数设置
  uint32_t openEnvPort = 5555; // 与zmpBridge交互使用的端口
  uint32_t simSeed = 1;

  // 仿真时长设置
  uint32_t maxStep = 10; // 最大仿真步数
  double envStepTime = 5; // 每一步时间长度

  // 流量工程默认仿真参数设置
  std::string routingMethod = "rl"; // 指定使用的路由规则[rl, ospf]
  std::string adjacencyMatrixStr = "[0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0]"; // 邻接矩阵
  std::string trafficMatrixStr = "[{/src/:0,/rate/:4,/dst/:1}]"; // TM

  CommandLine cmd;
  // required parameters for OpenEnv interface
  cmd.AddValue ("openEnvPort", "Port number for OpenEnv env. Default: 5555", openEnvPort);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  cmd.AddValue ("maxStep", "Simulation max steps. Default: 10", maxStep);
  // optional parameters
  cmd.AddValue ("routingMethod", "Ipv4 Routing Method, rl or ospf. Default: rl", routingMethod);
  cmd.AddValue ("trafficMatrix",
                "Assigned O-D TM, json type. Default: [{/src/:1,/rate/:4,/dst/:3}, "
                "{/src/:2,/rate/:1,/dst/:3}]",
                trafficMatrixStr);
  cmd.AddValue ("adjacencyMatrix",
                "Neighbor Matrix, json type, using -1 replace infinity. Defatult: "
                "[0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0]",
                adjacencyMatrixStr);
  cmd.Parse (argc, argv);
  // 为了确保传输json格式，需要进行一定的替换
  replace (trafficMatrixStr.begin (), trafficMatrixStr.end (), '/', '"');

  double simulationTime = envStepTime * maxStep; // 计算总仿真时长
  Time endTime = Seconds (simulationTime + 5.0); // 确定停止时间

  // 打印参数信息
  NS_LOG_UNCOND ("Ns3Env parameters:");
  NS_LOG_UNCOND ("--openEnvPort: " << openEnvPort);
  NS_LOG_UNCOND ("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND ("--maxStep: " << maxStep);
  NS_LOG_UNCOND ("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND ("--trafficMatrix: " << trafficMatrixStr);
  NS_LOG_UNCOND ("--adjacencyMatrix: " << adjacencyMatrixStr);
  NS_LOG_UNCOND ("--seed: " << simSeed);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);

  Document root;

  // 初始化拓扑结构参数
  std::vector<int> adjacencyVec = std::vector<int> ();
  root.Parse (adjacencyMatrixStr.c_str ());
  for (auto &item : root.GetArray ())
    {
      auto neighbor = item.GetInt ();
      adjacencyVec.push_back (neighbor);
    }

  // 获取节点数目、有向边数目
  uint32_t nodeNum = sqrt (adjacencyVec.size ());
  uint32_t edgeNum = count (adjacencyVec.begin (), adjacencyVec.end (), 1);
  int adjacencyArray[nodeNum * nodeNum];
  std::copy (adjacencyVec.begin (), adjacencyVec.end (), adjacencyArray);

  // 根据节点数目创建节点备用
  NodeContainer nodes;
  nodes.Create (nodeNum);

  // 使用上述信息创建网络构建类
  Ptr<MyNetwork> myNetwork = CreateObject<MyNetwork> (nodes, routingMethod, simulationTime);

  // 初始化拓扑结构
  myNetwork->BuildTopology (adjacencyVec);

  // 配置流分析器
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.RLInstallAll ();
  Ptr<Ipv4FlowClassifier> flowClassifier =
      DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());

  // 根据业务TM配置应用层
  root.Parse (trafficMatrixStr.c_str ());
  for (auto &item : root.GetArray ())
    {
      auto traffic = item.GetObject ();
      myNetwork->AddApplication (traffic["src"].GetInt (), traffic["dst"].GetInt (),
                                 traffic["rate"].GetDouble ());
    }

  if (routingMethod == "rl")
    {
      Ipv4RLRoutingHelper::InitializeRouteDatabase (adjacencyArray, nodes);
    }
  else
    {
      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

  // OpenEnv Env
  Ptr<OpenEnvInterface> openEnvInterface = CreateObject<OpenEnvInterface> (openEnvPort);
  Ptr<MyOpenEnv> myOpenEnv = CreateObject<MyOpenEnv> (Seconds (envStepTime), nodes, edgeNum, maxStep);
  myOpenEnv->SetFlowMonitor (flowMonitor);
  myOpenEnv->SetFlowClassifier (flowClassifier);
  myOpenEnv->SetAdjacencyVec(adjacencyVec);
  myOpenEnv->SetFlowVec (myNetwork->GetFlowVec ());
  myOpenEnv->SetOpenEnvInterface (openEnvInterface);

  // 从client启动开始计时
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime + 2));
  Simulator::Run ();

  NS_LOG_UNCOND ("Simulation stop");
  openEnvInterface->NotifySimulationEnd ();
  Simulator::Destroy ();
}

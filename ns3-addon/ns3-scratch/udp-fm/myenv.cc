/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Technische Universität Berlin
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
 * Author: Piotr Gawlowicz <gawlowicz@tkn.tu-berlin.de>
 */

#include "myenv.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/node-list.h"
#include "ns3/log.h"
#include <sstream>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyOpenEnv");

NS_OBJECT_ENSURE_REGISTERED (MyOpenEnv);

MyOpenEnv::MyOpenEnv ()
{
  NS_LOG_FUNCTION (this);
  m_interval = Seconds (0.1);
  m_needGameOver = true;
  Simulator::Schedule (Seconds (0.0), &MyOpenEnv::ScheduleNextStateRead, this);
}

MyOpenEnv::MyOpenEnv (Time stepTime, NodeContainer nodes, uint32_t edgeNum, uint32_t maxStep)
{
  NS_LOG_FUNCTION (this);
  m_interval = stepTime;
  m_needGameOver = true;
  m_nodes = nodes;
  m_edgeNum = edgeNum;
  m_maxStep = maxStep;

  Simulator::Schedule (Seconds (7.0), &MyOpenEnv::ScheduleNextStateRead, this);
}

void
MyOpenEnv::ScheduleNextStateRead ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (m_interval, &MyOpenEnv::ScheduleNextStateRead, this);
  Notify ();
}

MyOpenEnv::~MyOpenEnv ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MyOpenEnv::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyOpenEnv")
                          .SetParent<OpenEnvAbstract> ()
                          .SetGroupName ("OpenEnv")
                          .AddConstructor<MyOpenEnv> ();
  return tid;
}

void
MyOpenEnv::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
MyOpenEnv::SetAdjacencyVec (std::vector<int> adjacencyVec)
{
  m_adjacencyVec = adjacencyVec;
}

void
MyOpenEnv::SetFlowMonitor (Ptr<FlowMonitor> flowMonitor)
{
  m_flowMonitor = flowMonitor;
}

void
MyOpenEnv::SetFlowClassifier (Ptr<Ipv4FlowClassifier> flowClassifier)
{
  m_flowClassifier = flowClassifier;
}

void
MyOpenEnv::SetFlowVec (FlowVec flowVec)
{
  m_flowVec = flowVec;
}

/*
Define observation space
*/
Ptr<OpenEnvSpace>
MyOpenEnv::GetObservationSpace ()
{
  uint32_t nodeNum = m_nodes.GetN ();
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {
      nodeNum * nodeNum,
  };

  std::string dtype = TypeNameGet<uint32_t> ();

  Ptr<OpenEnvBoxSpace> space = CreateObject<OpenEnvBoxSpace> (low, high, shape, dtype);

  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;

} // namespace ns3

/*
Define action space
*/
Ptr<OpenEnvSpace>
MyOpenEnv::GetActionSpace ()
{
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {
      m_edgeNum,
  };
  std::string dtype = TypeNameGet<double> ();

  Ptr<OpenEnvBoxSpace> space = CreateObject<OpenEnvBoxSpace> (low, high, shape, dtype);

  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool
MyOpenEnv::GetGameOver ()
{
  bool isGameOver = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == m_maxStep && m_needGameOver)
    {
      isGameOver = true;
    }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenEnvDataContainer>
MyOpenEnv::GetObservation ()
{
  uint32_t nodeNum = m_nodes.GetN ();

  // 创建一个记录上一次TM的static变量
  static std::vector<uint32_t> cumFrowardMatrix = std::vector<uint32_t> (nodeNum * nodeNum, 0);
  std::vector<uint32_t> sumFrowardMatrix = GetForwardMatrix();
  // 仅当需要看详细数据时打开
  // flowMonitor->SerializeToXmlFile ("myanal.xml", true, true);
  // 创建box
  std::vector<uint32_t> shape = {
      nodeNum * nodeNum,
  };
  Ptr<OpenEnvBoxContainer<uint32_t>> box = CreateObject<OpenEnvBoxContainer<uint32_t>> (shape);

  // 用cur-last得到最近一个仿真时段的TM
  for (uint32_t index = 0; index < nodeNum * nodeNum; index++)
    {
      box->AddValue (sumFrowardMatrix[index] - cumFrowardMatrix[index]);
    }
  // 结束之后cur -> last
  cumFrowardMatrix.assign (sumFrowardMatrix.begin (), sumFrowardMatrix.end ());
  NS_LOG_UNCOND ("MyGetObservation: " << box);
  return box;
}

/*
Define reward function
*/
float
MyOpenEnv::GetReward ()
{
  static float reward;
  static int64_t cumNanoDelay = 0; // 累计的时延
  static uint32_t cumPackets = 0; // 累计包数目
  FlowMonitor::FlowStatsContainer flowStatsContainer = m_flowMonitor->GetFlowStats ();
  std::map<FlowId, FlowMonitor::FlowStats>::iterator it;
  if (flowStatsContainer.size () == 0)
    {
      return 0.0f;
    }
  // 遍历flowStats计算至今为止的时延和包数目
  int64_t sumNanoDelay = 0;
  uint32_t sumPackets = 0;
  for (it = flowStatsContainer.begin (); it != flowStatsContainer.end (); it++)
    {
      sumNanoDelay += it->second.delaySum.GetNanoSeconds ();
      NS_ASSERT (it->second.rxPackets > 0);
      sumPackets += it->second.rxPackets;
    }
  // 计算时延，之后 sum -> cum
  float nanoAvgDelay = (sumNanoDelay - cumNanoDelay) / (sumPackets - cumPackets);
  cumNanoDelay = sumNanoDelay;
  cumPackets = sumPackets;

  // 计算奖励，返回
  reward = (-nanoAvgDelay) / 1000000; // 取负数，转换为毫秒单位
  NS_LOG_UNCOND ("MyGetReward: " << reward);
  return reward;
}

/*
Define extra info. Optional
*/
std::string
MyOpenEnv::GetExtraInfo ()
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND ("MyGetExtraInfo: " << myInfo);
  return myInfo;
}

/*
Execute received actions
*/
bool
MyOpenEnv::ExecuteActions (Ptr<OpenEnvDataContainer> action)
{
  //!< 这里只能用float而不能用double
  Ptr<OpenEnvBoxContainer<float>> box = DynamicCast<OpenEnvBoxContainer<float>> (action);
  NS_LOG_UNCOND ("传入的action是: " << box);

  uint32_t nodeNum = m_nodes.GetN ();

  // 设置权重矩阵
  std::vector<float> actionVec = box->GetData ();
  NS_LOG_UNCOND ("传入的action size是: " << actionVec.size ());
  NS_LOG_UNCOND ("需要的action size是: " << m_edgeNum);
  // 初始化权重矩阵
  double weightArray[nodeNum * nodeNum];
  auto iter = actionVec.begin ();
  for (uint32_t src = 0; src < nodeNum; src++)
    for (uint32_t dst = 0; dst < nodeNum; dst++)
      {
        {
          if (m_adjacencyVec[src * nodeNum + dst] == 1)
            {
              NS_ASSERT (iter != actionVec.end ());
              weightArray[src * nodeNum + dst] = (*iter);
              iter++;
            }
          else
            {
              weightArray[src * nodeNum + dst] = 0;
            }
        }
      }

  // 设置距离矩阵并重新计算路由
  Ipv4RLRoutingHelper::ComputeRoutingTables (weightArray);
  return true;
}

std::vector<uint32_t>
MyOpenEnv::GetForwardMatrix ()
{
  uint32_t nodeNum = m_nodes.GetN ();

  // 将FlowMonitor中的记录整理到forwardVec中
  std::vector<uint32_t> forwardVec = std::vector<uint32_t> (nodeNum * nodeNum, 0);
  FlowMonitor::FlowProbeContainer flowProbeContainer = m_flowMonitor->GetAllProbes ();
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
          Ptr<NetDevice> device = 0;
          for (uint32_t dindex = 0; dindex < m_nodes.Get (src)->GetNDevices (); dindex++)
            {
              device = m_nodes.Get (src)->GetDevice (dindex);
              if (device->GetIfIndex () == interface)
                {
                  break;
                }
              device = 0;
            }
          if (device == 0)
            {
              NS_LOG_UNCOND ("未找到device");
            }
          // 获取与其相连的channel，查找对端
          Ptr<Channel> channel = device->GetChannel ();
          if (channel == 0)
            {
              NS_LOG_UNCOND ("未找到channel");
            }
          // 因为是point-to-point信道，所以一定是两个device，一个是自己，另一个就是对端
          uint32_t remoteIndex = channel->GetDevice (0) == device ? 1 : 0;
          // 此时device已经没用了，可以用来记录对端device
          device = channel->GetDevice (remoteIndex);
          // 通过device查找对端Node
          uint32_t dst = device->GetNode ()->GetId ();
          forwardVec[src * nodeNum + dst] += sit->second.packets;
        }
    }
  // 返回FM
  return forwardVec;
}

} // namespace ns3
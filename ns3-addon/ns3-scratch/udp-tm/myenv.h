/*
 * @author: Jiawei Wu
 * @create time: 1970-01-01 08:00
 * @edit time: 2020-03-02 16:18
 * @FilePath: /simulator/udp-tm/myenv.h
 */
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

#ifndef MY_ENV_ENTITY_H
#define MY_ENV_ENTITY_H

#include "ns3/openenv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"

namespace ns3 {

class MyOpenEnv : public OpenEnvAbstract
{
public:
  typedef std::pair<uint32_t, uint32_t> NodePair; //!< 两个node（由index确定）可以确定一条有向边
  typedef std::vector<NodePair> FlowVec; //!< 记录FlowId和对应的node的关系的Vector

  MyOpenEnv ();
  MyOpenEnv (Time stepTime, NodeContainer nodes, uint32_t edgeNum, uint32_t maxStep);
  virtual ~MyOpenEnv ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  Ptr<OpenEnvSpace> GetActionSpace ();
  Ptr<OpenEnvSpace> GetObservationSpace ();
  bool GetGameOver ();
  Ptr<OpenEnvDataContainer> GetObservation ();
  float GetReward ();
  std::string GetExtraInfo ();
  bool ExecuteActions (Ptr<OpenEnvDataContainer> action);

  void SetAdjacencyVec (std::vector<int> adjacencyVec);
  void SetFlowMonitor (Ptr<FlowMonitor> flowMonitor);
  void SetFlowClassifier (Ptr<Ipv4FlowClassifier> Classifier);
  void SetFlowVec (FlowVec flowVec);

private:
  void ScheduleNextStateRead ();

  NodeContainer m_nodes;
  uint32_t m_edgeNum;
  uint32_t m_maxStep;
  std::vector<int> m_adjacencyVec;
  Ptr<FlowMonitor> m_flowMonitor;
  Ptr<Ipv4FlowClassifier> m_flowClassifier;
  FlowVec m_flowVec;

  bool m_needGameOver;
  Time m_interval;
};

} // namespace ns3

#endif // MY_ENV_ENTITY_H

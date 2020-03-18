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

#include <sys/types.h>
#include <unistd.h>
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/simulator.h"
#include "openenv_interface.h"
#include "openenv_abstract.h"
#include "container.h"
#include "spaces.h"
#include "messages.pb.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenEnvInterface");

NS_OBJECT_ENSURE_REGISTERED (OpenEnvInterface);


TypeId
OpenEnvInterface::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvInterface")
    .SetParent<Object> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvInterface> ()
    ;
  return tid;
}

Ptr<OpenEnvInterface>
OpenEnvInterface::Get (uint32_t port)
{
  NS_LOG_FUNCTION_NOARGS ();
  return *DoGet (port);
}

Ptr<OpenEnvInterface> *
OpenEnvInterface::DoGet (uint32_t port)
{
  NS_LOG_FUNCTION_NOARGS ();
  static Ptr<OpenEnvInterface> ptr = 0;
  if (ptr == 0)
    {
      ptr = CreateObject<OpenEnvInterface> (port);
      Config::RegisterRootNamespaceObject (ptr);
      Simulator::ScheduleDestroy (&OpenEnvInterface::Delete);
    }
  return &ptr;
}

void
OpenEnvInterface::Delete (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Config::UnregisterRootNamespaceObject (Get ());
  (*DoGet ()) = 0;
}

OpenEnvInterface::OpenEnvInterface(uint32_t port):
  m_port(port), m_zmq_context(1), m_zmq_socket(m_zmq_context, ZMQ_REQ),
  m_simEnd(false), m_stopEnvRequested(false), m_initSimMsgSent(false)
{
  NS_LOG_FUNCTION (this);
}

OpenEnvInterface::~OpenEnvInterface ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvInterface::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvInterface::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvInterface::SetGetActionSpaceCb(Callback< Ptr<OpenEnvSpace> > cb)
{
  NS_LOG_FUNCTION (this);
  m_actionSpaceCb = cb;
}

void
OpenEnvInterface::SetGetObservationSpaceCb(Callback< Ptr<OpenEnvSpace> > cb)
{
  NS_LOG_FUNCTION (this);
  m_observationSpaceCb = cb;
}

void
OpenEnvInterface::SetGetGameOverCb(Callback< bool > cb)
{
  NS_LOG_FUNCTION (this);
  m_gameOverCb = cb;
}

void
OpenEnvInterface::SetGetObservationCb(Callback< Ptr<OpenEnvDataContainer> > cb)
{
  NS_LOG_FUNCTION (this);
  m_obsCb = cb;
}

void
OpenEnvInterface::SetGetRewardCb(Callback<float> cb)
{
  NS_LOG_FUNCTION (this);
  m_rewardCb = cb;
}

void
OpenEnvInterface::SetGetExtraInfoCb(Callback<std::string> cb)
{
  NS_LOG_FUNCTION (this);
  m_extraInfoCb = cb;
}

void
OpenEnvInterface::SetExecuteActionsCb(Callback<bool, Ptr<OpenEnvDataContainer> > cb)
{
  NS_LOG_FUNCTION (this);
  m_actionCb = cb;
}

void 
OpenEnvInterface::Init()
{
  NS_LOG_FUNCTION (this);
  // do not send init msg twice
  if (m_initSimMsgSent) {
    return;
  }
  m_initSimMsgSent = true;

  std::string connectAddr = "tcp://localhost:" + std::to_string(m_port);
  zmq_connect ((void*)m_zmq_socket, connectAddr.c_str());

  Ptr<OpenEnvSpace> obsSpace = GetObservationSpace();
  Ptr<OpenEnvSpace> actionSpace = GetActionSpace();

  NS_LOG_UNCOND("Simulation process id: " << ::getpid() << " (parent (waf shell) id: " << ::getppid() << ")");
  NS_LOG_UNCOND("Waiting for Python process to connect on port: "<< connectAddr);
  NS_LOG_UNCOND("Please start proper Python Env Agent");

  ns3openenv::SimInitMsg simInitMsg;
  simInitMsg.set_simprocessid(::getpid());
  simInitMsg.set_wafshellprocessid(::getppid());

  if (obsSpace) {
    ns3openenv::SpaceDescription spaceDesc;
    spaceDesc = obsSpace->GetSpaceDescription();
    simInitMsg.mutable_obsspace()->CopyFrom(spaceDesc);
  }

  if (actionSpace) {
    ns3openenv::SpaceDescription spaceDesc;
    spaceDesc = actionSpace->GetSpaceDescription();
    simInitMsg.mutable_actspace()->CopyFrom(spaceDesc);
  }

  // send init msg to python
  zmq::message_t request(simInitMsg.ByteSize());;
  simInitMsg.SerializeToArray(request.data(), simInitMsg.ByteSize());
  m_zmq_socket.send (request);

  // receive init ack msg form python
  ns3openenv::SimInitAck simInitAck;
  zmq::message_t reply;
  m_zmq_socket.recv (&reply);
  simInitAck.ParseFromArray(reply.data(), reply.size());

  bool done = simInitAck.done();
  NS_LOG_DEBUG("Sim Init Ack: " << done);

  bool stopSim = simInitAck.stopsimreq();
  if (stopSim) {
    NS_LOG_DEBUG("---Stop requested: " << stopSim);
    m_stopEnvRequested = true;
    Simulator::Stop();
    Simulator::Destroy ();
    std::exit(0);
  }
}

void
OpenEnvInterface::NotifyCurrentState()
{
  NS_LOG_FUNCTION (this);

  if (!m_initSimMsgSent) {
    Init();
  }

  if (m_stopEnvRequested) {
    return;
  }

  // collect current env state
  Ptr<OpenEnvDataContainer> obsDataContainer = GetObservation();
  float reward = GetReward();
  bool isGameOver = IsGameOver();
  std::string extraInfo = GetExtraInfo();

  ns3openenv::EnvStateMsg envStateMsg;
  // observation
  ns3openenv::DataContainer obsDataContainerPbMsg;
  if (obsDataContainer) {
    obsDataContainerPbMsg = obsDataContainer->GetDataContainerPbMsg();
    envStateMsg.mutable_obsdata()->CopyFrom(obsDataContainerPbMsg);
  }
  // reward
  envStateMsg.set_reward(reward);
  // game over
  envStateMsg.set_isgameover(false);
  if (isGameOver)
  {
    envStateMsg.set_isgameover(true);
    if (m_simEnd) {
      envStateMsg.set_reason(ns3openenv::EnvStateMsg::SimulationEnd);
    } else {
      envStateMsg.set_reason(ns3openenv::EnvStateMsg::GameOver);
    }
  }

  // extra info
  envStateMsg.set_info(extraInfo);

  // send env state msg to python
  zmq::message_t request(envStateMsg.ByteSize());;
  envStateMsg.SerializeToArray(request.data(), envStateMsg.ByteSize());
  m_zmq_socket.send (request);

  // receive act msg form python
  ns3openenv::EnvActMsg envActMsg;
  zmq::message_t reply;
  m_zmq_socket.recv (&reply);
  envActMsg.ParseFromArray(reply.data(), reply.size());

  if (m_simEnd) {
    // if sim end only rx ms and quit
    return;
  }

  bool stopSim = envActMsg.stopsimreq();
  if (stopSim) {
    NS_LOG_DEBUG("---Stop requested: " << stopSim);
    m_stopEnvRequested = true;
    Simulator::Stop();
    Simulator::Destroy ();
    std::exit(0);
  }

  // first step after reset is called without actions, just to get current state
  ns3openenv::DataContainer actDataContainerPbMsg = envActMsg.actdata();
  Ptr<OpenEnvDataContainer> actDataContainer = OpenEnvDataContainer::CreateFromDataContainerPbMsg(actDataContainerPbMsg);
  ExecuteActions(actDataContainer);

}

void
OpenEnvInterface::WaitForStop()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_UNCOND("Wait for stop message");
  NotifyCurrentState();
}

void
OpenEnvInterface::NotifySimulationEnd()
{
  NS_LOG_FUNCTION (this);
  m_simEnd = true;
  if (m_initSimMsgSent) {
    WaitForStop();
  }
}

bool
OpenEnvInterface::IsGameOver()
{
  NS_LOG_FUNCTION (this);
  bool gameOver = false;
  if (!m_gameOverCb.IsNull())
  {
    gameOver = m_gameOverCb();
  }
  return (gameOver || m_simEnd);
}

Ptr<OpenEnvSpace>
OpenEnvInterface::GetActionSpace()
{
  NS_LOG_FUNCTION (this);
  Ptr<OpenEnvSpace> actionSpace;
  if (!m_actionSpaceCb.IsNull())
  {
    actionSpace = m_actionSpaceCb();
  }
  return actionSpace;
}

Ptr<OpenEnvSpace>
OpenEnvInterface::GetObservationSpace()
{
  NS_LOG_FUNCTION (this);
  Ptr<OpenEnvSpace> obsSpace;
  if (!m_observationSpaceCb.IsNull())
  {
    obsSpace = m_observationSpaceCb();
  }
  return obsSpace;
}

Ptr<OpenEnvDataContainer>
OpenEnvInterface::GetObservation()
{
  NS_LOG_FUNCTION (this);
  Ptr<OpenEnvDataContainer>  obs;
  if (!m_obsCb.IsNull())
  {
    obs = m_obsCb();
  }
  return obs;
}

float
OpenEnvInterface::GetReward()
{
  NS_LOG_FUNCTION (this);
  float reward = 0.0;
  if (!m_rewardCb.IsNull())
  {
    reward = m_rewardCb();
  }
  return reward;
}

std::string
OpenEnvInterface::GetExtraInfo()
{
  NS_LOG_FUNCTION (this);
  std::string info;
  if (!m_extraInfoCb.IsNull())
  {
    info = m_extraInfoCb();
  }
  return info;
}

bool
OpenEnvInterface::ExecuteActions(Ptr<OpenEnvDataContainer> action)
{
  NS_LOG_FUNCTION (this);
  bool reply = false;
  if (!m_actionCb.IsNull())
  {
    reply = m_actionCb(action);
  }
  return reply;
}

void
OpenEnvInterface::Notify(Ptr<OpenEnvAbstract> entity)
{
  NS_LOG_FUNCTION (this);

  SetGetGameOverCb( MakeCallback (&OpenEnvAbstract::GetGameOver, entity) );
  SetGetObservationCb( MakeCallback (&OpenEnvAbstract::GetObservation, entity) );
  SetGetRewardCb( MakeCallback (&OpenEnvAbstract::GetReward, entity) );
  SetGetExtraInfoCb( MakeCallback (&OpenEnvAbstract::GetExtraInfo, entity) );
  SetExecuteActionsCb( MakeCallback (&OpenEnvAbstract::ExecuteActions, entity) );

  NotifyCurrentState();
}

}


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

#ifndef OPENENV_INTERFACE_H
#define OPENENV_INTERFACE_H

#include "ns3/object.h"
#include <zmq.hpp>

namespace ns3 {

class OpenEnvSpace;
class OpenEnvDataContainer;
class OpenEnvAbstract;

class OpenEnvInterface : public Object
{
public:
  static Ptr<OpenEnvInterface> Get (uint32_t port=5555);

  OpenEnvInterface (uint32_t port=5555);
  virtual ~OpenEnvInterface ();

  static TypeId GetTypeId ();

  void Init();
  void NotifyCurrentState();
  void WaitForStop();

  void NotifySimulationEnd();

  Ptr<OpenEnvSpace> GetActionSpace();
  Ptr<OpenEnvSpace> GetObservationSpace();
  Ptr<OpenEnvDataContainer> GetObservation();
  float GetReward();
  bool IsGameOver();
  std::string GetExtraInfo();
  bool ExecuteActions(Ptr<OpenEnvDataContainer> action);

  void SetGetActionSpaceCb(Callback< Ptr<OpenEnvSpace> > cb);
  void SetGetObservationSpaceCb(Callback< Ptr<OpenEnvSpace> > cb);
  void SetGetObservationCb(Callback< Ptr<OpenEnvDataContainer> > cb);
  void SetGetRewardCb(Callback<float> cb);
  void SetGetGameOverCb(Callback< bool > cb);
  void SetGetExtraInfoCb(Callback<std::string> cb);
  void SetExecuteActionsCb(Callback<bool, Ptr<OpenEnvDataContainer> > cb);

  void Notify(Ptr<OpenEnvAbstract> entity);

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:
  static Ptr<OpenEnvInterface> *DoGet (uint32_t port=5555);
  static void Delete (void);

  uint32_t m_port;
  zmq::context_t m_zmq_context;
  zmq::socket_t m_zmq_socket;

  bool m_simEnd;
  bool m_stopEnvRequested;
  bool m_initSimMsgSent;

  Callback< Ptr<OpenEnvSpace> > m_actionSpaceCb;
  Callback< Ptr<OpenEnvSpace> > m_observationSpaceCb;
  Callback< bool > m_gameOverCb;
  Callback< Ptr<OpenEnvDataContainer> > m_obsCb;
  Callback<float> m_rewardCb;
  Callback<std::string> m_extraInfoCb;
  Callback<bool, Ptr<OpenEnvDataContainer> > m_actionCb;
};

} // end of namespace ns3

#endif /* OPENENV_INTERFACE_H */


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

namespace ns3 {

class MyOpenEnv : public OpenEnvAbstract
{
public:
  MyOpenEnv ();
  MyOpenEnv (Time stepTime);
  virtual ~MyOpenEnv ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  Ptr<OpenEnvSpace> GetActionSpace();
  Ptr<OpenEnvSpace> GetObservationSpace();
  bool GetGameOver();
  Ptr<OpenEnvDataContainer> GetObservation();
  float GetReward();
  std::string GetExtraInfo();
  bool ExecuteActions(Ptr<OpenEnvDataContainer> action);

private:
  void ScheduleNextStateRead();

  Time m_interval;
};

}


#endif // MY_ENV_ENTITY_H

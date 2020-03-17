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

#ifndef OPENENV_ENV_H
#define OPENENV_ENV_H

#include "ns3/object.h"

namespace ns3 {

class OpenEnvSpace;
class OpenEnvDataContainer;
class OpenEnvInterface;

class OpenEnvAbstract : public Object
{
public:
  OpenEnvAbstract ();
  virtual ~OpenEnvAbstract ();

  static TypeId GetTypeId ();

  virtual Ptr<OpenEnvSpace> GetActionSpace() = 0;
  virtual Ptr<OpenEnvSpace> GetObservationSpace() = 0;
  // TODO:  get all in one function like below, do we need it?
  //virtual void GetEnvState(Ptr<OpenEnvDataContainer>  &obs, float &reward, bool &done, std::string &info) = 0;
  virtual bool GetGameOver() = 0;
  virtual Ptr<OpenEnvDataContainer> GetObservation() = 0;
  virtual float GetReward() = 0;
  virtual std::string GetExtraInfo() = 0;
  virtual bool ExecuteActions(Ptr<OpenEnvDataContainer> action) = 0;

  void SetOpenEnvInterface(Ptr<OpenEnvInterface> openEnvInterface);
  void Notify();
  void NotifySimulationEnd();


protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  Ptr<OpenEnvInterface> m_openEnvInterface;
private:

};

} // end of namespace ns3

#endif /* OPENENV_ENV_H */
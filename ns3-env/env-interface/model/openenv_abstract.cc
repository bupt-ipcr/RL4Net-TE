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

#include "ns3/log.h"
#include "ns3/object.h"
#include "openenv_abstract.h"
#include "container.h"
#include "spaces.h"
#include "openenv_interface.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (OpenEnvAbstract);

NS_LOG_COMPONENT_DEFINE ("OpenEnvAbstract");

TypeId
OpenEnvAbstract::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenEnvAbstract")
    .SetParent<Object> ()
    .SetGroupName ("OpenEnv")
    ;
  return tid;
}

OpenEnvAbstract::OpenEnvAbstract()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvAbstract::~OpenEnvAbstract ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvAbstract::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvAbstract::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvAbstract::SetOpenEnvInterface(Ptr<OpenEnvInterface> openEnvInterface)
{
  NS_LOG_FUNCTION (this);
  m_openEnvInterface = openEnvInterface;
  openEnvInterface->SetGetActionSpaceCb( MakeCallback (&OpenEnvAbstract::GetActionSpace, this) );
  openEnvInterface->SetGetObservationSpaceCb( MakeCallback (&OpenEnvAbstract::GetObservationSpace, this) );
  openEnvInterface->SetGetGameOverCb( MakeCallback (&OpenEnvAbstract::GetGameOver, this) );
  openEnvInterface->SetGetObservationCb( MakeCallback (&OpenEnvAbstract::GetObservation, this) );
  openEnvInterface->SetGetRewardCb( MakeCallback (&OpenEnvAbstract::GetReward, this) );
  openEnvInterface->SetGetExtraInfoCb( MakeCallback (&OpenEnvAbstract::GetExtraInfo, this) );
  openEnvInterface->SetExecuteActionsCb( MakeCallback (&OpenEnvAbstract::ExecuteActions, this) );
}

void
OpenEnvAbstract::Notify()
{
  NS_LOG_FUNCTION (this);
  if (m_openEnvInterface)
  {
    m_openEnvInterface->Notify(this);
  }
}

void
OpenEnvAbstract::NotifySimulationEnd()
{
  NS_LOG_FUNCTION (this);
  if (m_openEnvInterface)
  {
    m_openEnvInterface->NotifySimulationEnd();
  }
}

}
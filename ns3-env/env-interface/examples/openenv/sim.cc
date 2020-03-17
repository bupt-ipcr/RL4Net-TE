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

#include "ns3/core-module.h"
#include "ns3/openenv-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OpenEnv");

/*
Define observation space
*/
Ptr<OpenEnvSpace> MyGetObservationSpace(void)
{
  uint32_t nodeNum = 5;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenEnvBoxSpace> space = CreateObject<OpenEnvBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

/*
Define action space
*/
Ptr<OpenEnvSpace> MyGetActionSpace(void)
{
  uint32_t nodeNum = 5;

  Ptr<OpenEnvDiscreteSpace> space = CreateObject<OpenEnvDiscreteSpace> (nodeNum);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool MyGetGameOver(void)
{

  bool isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == 10 && test) {
      isGameOver = true;
  }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenEnvDataContainer> MyGetObservation(void)
{
  uint32_t nodeNum = 5;
  uint32_t low = 0.0;
  uint32_t high = 10.0;
  Ptr<UniformRandomVariable> rngInt = CreateObject<UniformRandomVariable> ();

  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenEnvBoxContainer<uint32_t> > box = CreateObject<OpenEnvBoxContainer<uint32_t> >(shape);

  // generate random data
  for (uint32_t i = 0; i<nodeNum; i++){
    uint32_t value = rngInt->GetInteger(low, high);
    box->AddValue(value);
  }

  NS_LOG_UNCOND ("MyGetObservation: " << box);
  return box;
}

/*
Define reward function
*/
float MyGetReward(void)
{
  static float reward = 0.0;
  reward += 1;
  return reward;
}

/*
Define extra info. Optional
*/
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}


/*
Execute received actions
*/
bool MyExecuteActions(Ptr<OpenEnvDataContainer> action)
{
  Ptr<OpenEnvDiscreteContainer> discrete = DynamicCast<OpenEnvDiscreteContainer>(action);
  NS_LOG_UNCOND ("MyExecuteActions: " << action);
  return true;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenEnvInterface> openEnv)
{
  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openEnv);
  openEnv->NotifyCurrentState();
}

int
main (int argc, char *argv[])
{
  // Parameters of the scenario
  uint32_t simSeed = 1;
  double simulationTime = 1; //seconds
  double envStepTime = 0.1; //seconds, ns3env env step time interval
  uint32_t openEnvPort = 5555;
  uint32_t testArg = 0;

  CommandLine cmd;
  // required parameters for OpenEnv interface
  cmd.AddValue ("openEnvPort", "Port number for OpenEnv env. Default: 5555", openEnvPort);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  // optional parameters
  cmd.AddValue ("simTime", "Simulation time in seconds. Default: 10s", simulationTime);
  cmd.AddValue ("testArg", "Extra simulation argument. Default: 0", testArg);
  cmd.Parse (argc, argv);

  NS_LOG_UNCOND("Ns3Env parameters:");
  NS_LOG_UNCOND("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND("--openEnvPort: " << openEnvPort);
  NS_LOG_UNCOND("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND("--seed: " << simSeed);
  NS_LOG_UNCOND("--testArg: " << testArg);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);

  // OpenEnv Env
  Ptr<OpenEnvInterface> openEnv = CreateObject<OpenEnvInterface> (openEnvPort);
  openEnv->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openEnv->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openEnv->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openEnv->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  openEnv->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openEnv->SetGetExtraInfoCb( MakeCallback (&MyGetExtraInfo) );
  openEnv->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );
  Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openEnv);

  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  NS_LOG_UNCOND ("Simulation stop");

  openEnv->NotifySimulationEnd();
  Simulator::Destroy ();

}

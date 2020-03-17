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
#include "container.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenEnvDataContainer");

NS_OBJECT_ENSURE_REGISTERED (OpenEnvDataContainer);


TypeId
OpenEnvDataContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenEnvDataContainer")
    .SetParent<Object> ()
    .SetGroupName ("OpenEnv")
    ;
  return tid;
}

OpenEnvDataContainer::OpenEnvDataContainer()
{
  //NS_LOG_FUNCTION (this);
}

OpenEnvDataContainer::~OpenEnvDataContainer ()
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDataContainer::DoDispose (void)
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDataContainer::DoInitialize (void)
{
  //NS_LOG_FUNCTION (this);
}

Ptr<OpenEnvDataContainer>
OpenEnvDataContainer::CreateFromDataContainerPbMsg(ns3openenv::DataContainer &dataContainerPbMsg)
{
  Ptr<OpenEnvDataContainer> actDataContainer;

  if (dataContainerPbMsg.type() == ns3openenv::Discrete)
  {
    ns3openenv::DiscreteDataContainer discreteContainerPbMsg;
    dataContainerPbMsg.data().UnpackTo(&discreteContainerPbMsg);

    Ptr<OpenEnvDiscreteContainer> discrete = CreateObject<OpenEnvDiscreteContainer>();
    discrete->SetValue(discreteContainerPbMsg.data());
    actDataContainer = discrete;
  }
  else if (dataContainerPbMsg.type() == ns3openenv::Box)
  {
    ns3openenv::BoxDataContainer boxContainerPbMsg;
    dataContainerPbMsg.data().UnpackTo(&boxContainerPbMsg);

    if (boxContainerPbMsg.dtype() == ns3openenv::INT) {
      Ptr<OpenEnvBoxContainer<int32_t> > box = CreateObject<OpenEnvBoxContainer<int32_t> >();
      std::vector<int32_t> myData;
      myData.assign(boxContainerPbMsg.intdata().begin(), boxContainerPbMsg.intdata().end());
      box->SetData(myData);
      actDataContainer = box;

    } else if (boxContainerPbMsg.dtype() == ns3openenv::UINT) {
      Ptr<OpenEnvBoxContainer<uint32_t> > box = CreateObject<OpenEnvBoxContainer<uint32_t> >();
      std::vector<uint32_t> myData;
      myData.assign(boxContainerPbMsg.uintdata().begin(), boxContainerPbMsg.uintdata().end());
      box->SetData(myData);
      actDataContainer = box;

    } else if (boxContainerPbMsg.dtype() == ns3openenv::FLOAT) {
      Ptr<OpenEnvBoxContainer<float> > box = CreateObject<OpenEnvBoxContainer<float> >();
      std::vector<float> myData;
      myData.assign(boxContainerPbMsg.floatdata().begin(), boxContainerPbMsg.floatdata().end());
      box->SetData(myData);
      actDataContainer = box;

    } else if (boxContainerPbMsg.dtype() == ns3openenv::DOUBLE) {
      Ptr<OpenEnvBoxContainer<double> > box = CreateObject<OpenEnvBoxContainer<double> >();
      std::vector<double> myData;
      myData.assign(boxContainerPbMsg.doubledata().begin(), boxContainerPbMsg.doubledata().end());
      box->SetData(myData);
      actDataContainer = box;

    } else {
      Ptr<OpenEnvBoxContainer<float> > box = CreateObject<OpenEnvBoxContainer<float> >();
      std::vector<float> myData;
      myData.assign(boxContainerPbMsg.floatdata().begin(), boxContainerPbMsg.floatdata().end());
      box->SetData(myData);
      actDataContainer = box;
    }
  }
  else if (dataContainerPbMsg.type() == ns3openenv::Tuple)
  {
    Ptr<OpenEnvTupleContainer> tupleData = CreateObject<OpenEnvTupleContainer> ();

    ns3openenv::TupleDataContainer tupleContainerPbMsg;
    dataContainerPbMsg.data().UnpackTo(&tupleContainerPbMsg);

    std::vector< ns3openenv::DataContainer > elements;
    elements.assign(tupleContainerPbMsg.element().begin(), tupleContainerPbMsg.element().end());

    std::vector< ns3openenv::DataContainer >::iterator it;
    for(it=elements.begin();it!=elements.end();++it)
    {
      Ptr<OpenEnvDataContainer> subData = OpenEnvDataContainer::CreateFromDataContainerPbMsg(*it);
      tupleData->Add(subData);
    }

    actDataContainer = tupleData;
  }
  else if (dataContainerPbMsg.type() == ns3openenv::Dict)
  {
    Ptr<OpenEnvDictContainer> dictData = CreateObject<OpenEnvDictContainer> ();

    ns3openenv::DictDataContainer dictContainerPbMsg;
    dataContainerPbMsg.data().UnpackTo(&dictContainerPbMsg);

    std::vector< ns3openenv::DataContainer > elements;
    elements.assign(dictContainerPbMsg.element().begin(), dictContainerPbMsg.element().end());

    std::vector< ns3openenv::DataContainer >::iterator it;
    for(it=elements.begin();it!=elements.end();++it)
    {
      Ptr<OpenEnvDataContainer> subSpace = OpenEnvDataContainer::CreateFromDataContainerPbMsg(*it);
      dictData->Add((*it).name(), subSpace);
    }

    actDataContainer = dictData;
  }
  return actDataContainer;
}


TypeId
OpenEnvDiscreteContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenEnvDiscreteContainer")
    .SetParent<OpenEnvDataContainer> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvDiscreteContainer> ()
    ;
  return tid;
}

OpenEnvDiscreteContainer::OpenEnvDiscreteContainer()
{
  //NS_LOG_FUNCTION (this);
  m_n = 0;
}

OpenEnvDiscreteContainer::OpenEnvDiscreteContainer(uint32_t n)
{
  //NS_LOG_FUNCTION (this);
  m_n = n;
}

OpenEnvDiscreteContainer::~OpenEnvDiscreteContainer ()
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDiscreteContainer::DoDispose (void)
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDiscreteContainer::DoInitialize (void)
{
  //NS_LOG_FUNCTION (this);
}

ns3openenv::DataContainer
OpenEnvDiscreteContainer::GetDataContainerPbMsg()
{
  ns3openenv::DataContainer dataContainerPbMsg;
  ns3openenv::DiscreteDataContainer discreteContainerPbMsg;
  discreteContainerPbMsg.set_data(GetValue());

  dataContainerPbMsg.set_type(ns3openenv::Discrete);
  dataContainerPbMsg.mutable_data()->PackFrom(discreteContainerPbMsg);
  return dataContainerPbMsg;
}

bool
OpenEnvDiscreteContainer::SetValue(uint32_t value)
{
  m_value = value;
  return true;
}

uint32_t
OpenEnvDiscreteContainer::GetValue()
{
  return m_value;
}

void
OpenEnvDiscreteContainer::Print(std::ostream& where) const
{
  where << std::to_string(m_value);
}

TypeId
OpenEnvTupleContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenEnvTupleContainer")
    .SetParent<OpenEnvDataContainer> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvTupleContainer> ()
    ;
  return tid;
}

OpenEnvTupleContainer::OpenEnvTupleContainer()
{
  //NS_LOG_FUNCTION (this);
}

OpenEnvTupleContainer::~OpenEnvTupleContainer ()
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvTupleContainer::DoDispose (void)
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvTupleContainer::DoInitialize (void)
{
  //NS_LOG_FUNCTION (this);
}

ns3openenv::DataContainer
OpenEnvTupleContainer::GetDataContainerPbMsg()
{
  ns3openenv::DataContainer dataContainerPbMsg;
  dataContainerPbMsg.set_type(ns3openenv::Tuple);

  ns3openenv::TupleDataContainer tupleContainerPbMsg;

  std::vector< Ptr<OpenEnvDataContainer> >::iterator it;
  for (it=m_tuple.begin(); it!=m_tuple.end(); ++it)
  {
    Ptr<OpenEnvDataContainer> subSpace = *it;
    ns3openenv::DataContainer subDataContainer = subSpace->GetDataContainerPbMsg();

    tupleContainerPbMsg.add_element()->CopyFrom(subDataContainer);
  }

  dataContainerPbMsg.mutable_data()->PackFrom(tupleContainerPbMsg);
  return dataContainerPbMsg;
}

bool
OpenEnvTupleContainer::Add(Ptr<OpenEnvDataContainer> space)
{
  NS_LOG_FUNCTION (this);
  m_tuple.push_back(space);
  return true;
}

Ptr<OpenEnvDataContainer>
OpenEnvTupleContainer::Get(uint32_t idx)
{
  Ptr<OpenEnvDataContainer> data;

  if (idx < m_tuple.size())
  {
    data = m_tuple.at(idx);
  }

  return data;
}

void
OpenEnvTupleContainer::Print(std::ostream& where) const
{
  where << "Tuple(";

  std::vector< Ptr<OpenEnvDataContainer> >::const_iterator it;
  std::vector< Ptr<OpenEnvDataContainer> >::const_iterator it2;
  for (it=m_tuple.cbegin(); it!=m_tuple.cend(); ++it)
  {
    Ptr<OpenEnvDataContainer> subSpace = *it;
    subSpace->Print(where);

    it2 = it;
    it2++;
    if (it2 != m_tuple.end())
      where << ", ";
  }
  where << ")";
}


TypeId
OpenEnvDictContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenEnvDictContainer")
    .SetParent<OpenEnvDataContainer> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvDictContainer> ()
    ;
  return tid;
}

OpenEnvDictContainer::OpenEnvDictContainer()
{
  //NS_LOG_FUNCTION (this);
}

OpenEnvDictContainer::~OpenEnvDictContainer ()
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDictContainer::DoDispose (void)
{
  //NS_LOG_FUNCTION (this);
}

void
OpenEnvDictContainer::DoInitialize (void)
{
  //NS_LOG_FUNCTION (this);
}

ns3openenv::DataContainer
OpenEnvDictContainer::GetDataContainerPbMsg()
{
  ns3openenv::DataContainer dataContainerPbMsg;
  dataContainerPbMsg.set_type(ns3openenv::Dict);

  ns3openenv::DictDataContainer dictContainerPbMsg;

  std::map< std::string, Ptr<OpenEnvDataContainer> >::iterator it;
  for (it=m_dict.begin(); it!=m_dict.end(); ++it)
  {
    std::string name = it->first;
    Ptr<OpenEnvDataContainer> subSpace = it->second;

    ns3openenv::DataContainer subDataContainer = subSpace->GetDataContainerPbMsg();
    subDataContainer.set_name(name);

    dictContainerPbMsg.add_element()->CopyFrom(subDataContainer);
  }

  dataContainerPbMsg.mutable_data()->PackFrom(dictContainerPbMsg);
  return dataContainerPbMsg;
}

bool
OpenEnvDictContainer::Add(std::string key, Ptr<OpenEnvDataContainer> data)
{
  NS_LOG_FUNCTION (this);
  m_dict.insert(std::pair<std::string, Ptr<OpenEnvDataContainer> > (key, data));
  return true;
}

Ptr<OpenEnvDataContainer>
OpenEnvDictContainer::Get(std::string key)
{
  Ptr<OpenEnvDataContainer> data;
  std::map< std::string, Ptr<OpenEnvDataContainer> >::iterator it = m_dict.find(key);
  if ( it != m_dict.end() ) {
    data = it->second;
  }
  return data;
}

void
OpenEnvDictContainer::Print(std::ostream& where) const
{
  where << "Dict(";

  std::map< std::string, Ptr<OpenEnvDataContainer> >::const_iterator it;
  std::map< std::string, Ptr<OpenEnvDataContainer> >::const_iterator it2;
  for (it=m_dict.cbegin(); it!=m_dict.cend(); ++it)
  {
    std::string name = it->first;
    Ptr<OpenEnvDataContainer> subSpace = it->second;

    where << name << "=";
    subSpace->Print(where);

    it2 = it;
    it2++;
    if (it2 != m_dict.end())
      where << ", ";
  }
  where << ")";
}

}
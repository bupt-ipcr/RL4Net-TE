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

#include "ns3/object.h"
#include "ns3/log.h"
#include "spaces.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenEnvSpace");

NS_OBJECT_ENSURE_REGISTERED (OpenEnvSpace);


TypeId
OpenEnvSpace::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvSpace")
    .SetParent<Object> ()
    .SetGroupName ("OpenEnv")
    ;
  return tid;
}

OpenEnvSpace::OpenEnvSpace()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvSpace::~OpenEnvSpace ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvSpace::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvSpace::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}


TypeId
OpenEnvDiscreteSpace::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvDiscreteSpace")
    .SetParent<OpenEnvSpace> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvDiscreteSpace> ()
    ;
  return tid;
}

OpenEnvDiscreteSpace::OpenEnvDiscreteSpace()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvDiscreteSpace::OpenEnvDiscreteSpace(int n):
  m_n(n)
{
  NS_LOG_FUNCTION (this);
}

OpenEnvDiscreteSpace::~OpenEnvDiscreteSpace ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvDiscreteSpace::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvDiscreteSpace::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

int
OpenEnvDiscreteSpace::GetN (void)
{
  NS_LOG_FUNCTION (this);
  return m_n;
}

ns3openenv::SpaceDescription
OpenEnvDiscreteSpace::GetSpaceDescription()
{
  NS_LOG_FUNCTION (this);
  ns3openenv::SpaceDescription desc;
  desc.set_type(ns3openenv::Discrete);
  ns3openenv::DiscreteSpace discreteSpace;
  discreteSpace.set_n(GetN());
  desc.mutable_space()->PackFrom(discreteSpace);
  return desc;
}

void
OpenEnvDiscreteSpace::Print(std::ostream& where) const
{
  where << " DiscreteSpace N: " << m_n;
}

TypeId
OpenEnvBoxSpace::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvBoxSpace")
    .SetParent<OpenEnvSpace> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvBoxSpace> ()
    ;
  return tid;
}

OpenEnvBoxSpace::OpenEnvBoxSpace ()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvBoxSpace::OpenEnvBoxSpace (float low, float high, std::vector<uint32_t> shape, std::string dtype):
  m_low(low), m_high(high), m_shape(shape), m_dtypeName(dtype)
{
  NS_LOG_FUNCTION (this);
  SetDtype ();
}

OpenEnvBoxSpace::OpenEnvBoxSpace (std::vector<float> low, std::vector<float> high, std::vector<uint32_t> shape, std::string dtype):
  m_low(0), m_high(0), m_shape(shape), m_dtypeName(dtype), m_lowVec(low), m_highVec(high)

{
  NS_LOG_FUNCTION (this);
  SetDtype ();
} 

OpenEnvBoxSpace::~OpenEnvBoxSpace ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvBoxSpace::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvBoxSpace::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvBoxSpace::SetDtype ()
{
  std::string name = m_dtypeName;
  if (name == "int8_t" || name == "int16_t" || name == "int32_t" || name == "int64_t")
    m_dtype = ns3openenv::INT;
  else if (name == "uint8_t" || name == "uint16_t" || name == "uint32_t" || name == "uint64_t")
    m_dtype = ns3openenv::UINT;
  else if (name == "float")
    m_dtype = ns3openenv::FLOAT;
  else if (name == "double")
    m_dtype = ns3openenv::DOUBLE;
  else
    m_dtype = ns3openenv::FLOAT;
}

float
OpenEnvBoxSpace::GetLow()
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

float
OpenEnvBoxSpace::GetHigh()
{
  NS_LOG_FUNCTION (this);
  return m_high;
}

std::vector<uint32_t>
OpenEnvBoxSpace::GetShape()
{
  NS_LOG_FUNCTION (this);
  return m_shape;
}

ns3openenv::SpaceDescription
OpenEnvBoxSpace::GetSpaceDescription()
{
  NS_LOG_FUNCTION (this);
  ns3openenv::SpaceDescription desc;
  desc.set_type(ns3openenv::Box);

  ns3openenv::BoxSpace boxSpacePb;
  boxSpacePb.set_low(GetLow());
  boxSpacePb.set_high(GetHigh());

  std::vector<uint32_t> shape = GetShape();
  for (auto i = shape.begin(); i != shape.end(); ++i)
  {
    boxSpacePb.add_shape(*i);
  }

  boxSpacePb.set_dtype(m_dtype);
  desc.mutable_space()->PackFrom(boxSpacePb);
  return desc;
}

void
OpenEnvBoxSpace::Print(std::ostream& where) const
{
  where << " BoxSpace Low: " << m_low << " High: " << m_high << " Shape: (";

  for (auto i = m_shape.begin(); i != m_shape.end(); ++i)
  {
    where << *i << ",";
  }
  where << ") Dtype: " << m_dtypeName;
}


TypeId
OpenEnvTupleSpace::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvTupleSpace")
    .SetParent<OpenEnvSpace> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvTupleSpace> ()
    ;
  return tid;
}

OpenEnvTupleSpace::OpenEnvTupleSpace ()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvTupleSpace::~OpenEnvTupleSpace ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvTupleSpace::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvTupleSpace::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

bool
OpenEnvTupleSpace::Add(Ptr<OpenEnvSpace> space)
{
  NS_LOG_FUNCTION (this);
  m_tuple.push_back(space);
  return true;
}

Ptr<OpenEnvSpace>
OpenEnvTupleSpace::Get(uint32_t idx)
{
  NS_LOG_FUNCTION (this);
  Ptr<OpenEnvSpace> space;

  if (idx < m_tuple.size())
  {
    space = m_tuple.at(idx);
  }

  return space;
}

ns3openenv::SpaceDescription
OpenEnvTupleSpace::GetSpaceDescription()
{
  NS_LOG_FUNCTION (this);
  ns3openenv::SpaceDescription desc;
  desc.set_type(ns3openenv::Tuple);

  ns3openenv::TupleSpace tupleSpacePb;

  for (auto i = m_tuple.begin(); i != m_tuple.end(); ++i)
  {
    Ptr<OpenEnvSpace> subSpace = *i;
    ns3openenv::SpaceDescription subDesc = subSpace->GetSpaceDescription();
    tupleSpacePb.add_element()->CopyFrom(subDesc);
  }

  desc.mutable_space()->PackFrom(tupleSpacePb);
  return desc;
}

void
OpenEnvTupleSpace::Print(std::ostream& where) const
{
  where << " TupleSpace: " << std::endl;

  for (auto i = m_tuple.begin(); i != m_tuple.end(); ++i)
  {
    where << "---";
    (*i)->Print(where);
    where << std::endl;
  }
}


TypeId
OpenEnvDictSpace::GetTypeId (void)
{
  static TypeId tid = TypeId ("OpenEnvDictSpace")
    .SetParent<OpenEnvSpace> ()
    .SetGroupName ("OpenEnv")
    .AddConstructor<OpenEnvDictSpace> ()
    ;
  return tid;
}

OpenEnvDictSpace::OpenEnvDictSpace ()
{
  NS_LOG_FUNCTION (this);
}

OpenEnvDictSpace::~OpenEnvDictSpace ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvDictSpace::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
OpenEnvDictSpace::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

bool
OpenEnvDictSpace::Add(std::string key, Ptr<OpenEnvSpace> space)
{
  NS_LOG_FUNCTION (this);
  m_dict.insert(std::pair<std::string, Ptr<OpenEnvSpace> > (key, space));
  return true;
}

Ptr<OpenEnvSpace>
OpenEnvDictSpace::Get(std::string key)
{
  NS_LOG_FUNCTION (this);
  Ptr<OpenEnvSpace> space;
  std::map< std::string, Ptr<OpenEnvSpace> >::iterator it = m_dict.find(key);
  if ( it != m_dict.end() ) {
    space = it->second;
  }

  return space;
}

ns3openenv::SpaceDescription
OpenEnvDictSpace::GetSpaceDescription()
{
  NS_LOG_FUNCTION (this);
  ns3openenv::SpaceDescription desc;
  desc.set_type(ns3openenv::Dict);

  ns3openenv::DictSpace dictSpacePb;

  std::map< std::string, Ptr<OpenEnvSpace> >::iterator it;
  for (it=m_dict.begin(); it!=m_dict.end(); ++it)
  {
    std::string name = it->first;
    Ptr<OpenEnvSpace> subSpace = it->second;

    ns3openenv::SpaceDescription subDesc = subSpace->GetSpaceDescription();
    subDesc.set_name(name);

    dictSpacePb.add_element()->CopyFrom(subDesc);
  }

  desc.mutable_space()->PackFrom(dictSpacePb);
  return desc;
}

void
OpenEnvDictSpace::Print(std::ostream& where) const
{
  where << " DictSpace: " << std::endl;

  std::map< std::string, Ptr<OpenEnvSpace> > myMap = m_dict;
  std::map< std::string, Ptr<OpenEnvSpace> >::iterator it;
  for (it=myMap.begin(); it!=myMap.end(); ++it)
  {
    where << "---" << it->first << ":";
    it->second->Print(where);
    where << std::endl;
  }
}

}
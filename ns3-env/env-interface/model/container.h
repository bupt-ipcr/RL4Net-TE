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

#ifndef OPENENV_CONTAINER_H
#define OPENENV_CONTAINER_H

#include "ns3/object.h"
#include "ns3/type-name.h"
#include "messages.pb.h"

namespace ns3 {

class OpenEnvDataContainer : public Object
{
public:
  OpenEnvDataContainer ();
  virtual ~OpenEnvDataContainer ();

  static TypeId GetTypeId ();

  virtual ns3openenv::DataContainer GetDataContainerPbMsg() = 0;
  static Ptr<OpenEnvDataContainer> CreateFromDataContainerPbMsg(ns3openenv::DataContainer &dataContainer);

  virtual void Print(std::ostream& where) const = 0;
  friend std::ostream& operator<< (std::ostream& os, const Ptr<OpenEnvDataContainer> container)
  {
    container->Print(os);
    return os;
  }

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);
};


class OpenEnvDiscreteContainer : public OpenEnvDataContainer
{
public:
  OpenEnvDiscreteContainer ();
  OpenEnvDiscreteContainer (uint32_t n);
  virtual ~OpenEnvDiscreteContainer ();

  static TypeId GetTypeId ();

  virtual ns3openenv::DataContainer GetDataContainerPbMsg();

  virtual void Print(std::ostream& where) const;
  friend std::ostream& operator<< (std::ostream& os, const Ptr<OpenEnvDiscreteContainer> container)
  {
    container->Print(os);
    return os;
  }

  bool SetValue(uint32_t value);
  uint32_t GetValue();

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  uint32_t m_n;
  uint32_t m_value;
};

template <typename T = float>
class OpenEnvBoxContainer : public OpenEnvDataContainer
{
public:
  OpenEnvBoxContainer ();
  OpenEnvBoxContainer (std::vector<uint32_t> shape);
  virtual ~OpenEnvBoxContainer ();

  static TypeId GetTypeId ();

  virtual ns3openenv::DataContainer GetDataContainerPbMsg();

  virtual void Print(std::ostream& where) const;
  friend std::ostream& operator<< (std::ostream& os, const Ptr<OpenEnvBoxContainer> container)
  {
    container->Print(os);
    return os;
  }

  bool AddValue(T value);
  T GetValue(uint32_t idx);

  bool SetData(std::vector<T> data);
  std::vector<T> GetData();

  std::vector<uint32_t> GetShape();

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:
  void SetDtype();
	std::vector<uint32_t> m_shape;
	ns3openenv::Dtype m_dtype;
	std::vector<T> m_data;
};

template <typename T>
TypeId
OpenEnvBoxContainer<T>::GetTypeId (void)
{
  std::string name = TypeNameGet<T> ();
  static TypeId tid = TypeId (("ns3::OpenEnvBoxContainer<" + name + ">").c_str ())
    .SetParent<Object> ()
    .SetGroupName ("OpenEnv")
    .template AddConstructor<OpenEnvBoxContainer<T> > ()
    ;
  return tid;
}

template <typename T>
OpenEnvBoxContainer<T>::OpenEnvBoxContainer()
{
 SetDtype();
}

template <typename T>
OpenEnvBoxContainer<T>::OpenEnvBoxContainer(std::vector<uint32_t> shape):
	m_shape(shape)
{
  SetDtype();
}

template <typename T>
OpenEnvBoxContainer<T>::~OpenEnvBoxContainer ()
{
}

template <typename T>
void
OpenEnvBoxContainer<T>::SetDtype ()
{
  std::string name = TypeNameGet<T> ();
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

template <typename T>
void
OpenEnvBoxContainer<T>::DoDispose (void)
{
}

template <typename T>
void
OpenEnvBoxContainer<T>::DoInitialize (void)
{
}

template <typename T>
ns3openenv::DataContainer
OpenEnvBoxContainer<T>::GetDataContainerPbMsg()
{
  ns3openenv::DataContainer dataContainerPbMsg;
  ns3openenv::BoxDataContainer boxContainerPbMsg;

  std::vector<uint32_t> shape = GetShape();
  *boxContainerPbMsg.mutable_shape() = {shape.begin(), shape.end()};


  boxContainerPbMsg.set_dtype(m_dtype);
  std::vector<T> data = GetData();

  if (m_dtype == ns3openenv::INT) {
    *boxContainerPbMsg.mutable_intdata() = {data.begin(), data.end()};

  } else if (m_dtype == ns3openenv::UINT) {
    *boxContainerPbMsg.mutable_uintdata() = {data.begin(), data.end()};

  } else if (m_dtype == ns3openenv::FLOAT) {
    *boxContainerPbMsg.mutable_floatdata() = {data.begin(), data.end()};

  } else if (m_dtype == ns3openenv::DOUBLE) {
    *boxContainerPbMsg.mutable_doubledata() = {data.begin(), data.end()};

  } else {
    *boxContainerPbMsg.mutable_floatdata() = {data.begin(), data.end()};
  }

  dataContainerPbMsg.set_type(ns3openenv::Box);
  dataContainerPbMsg.mutable_data()->PackFrom(boxContainerPbMsg);
  return dataContainerPbMsg;
}

template <typename T>
bool
OpenEnvBoxContainer<T>::AddValue(T value)
{
  m_data.push_back(value);
  return true;
}

template <typename T>
T
OpenEnvBoxContainer<T>::GetValue(uint32_t idx)
{
  T data = 0;
  if (idx < m_data.size())
  {
    data = m_data.at(idx);
  }
  return data;
}

template <typename T>
bool
OpenEnvBoxContainer<T>::SetData(std::vector<T> data)
{
  m_data = data;
  return true;
}

template <typename T>
std::vector<uint32_t>
OpenEnvBoxContainer<T>::GetShape()
{
  return m_shape;
}

template <typename T>
std::vector<T>
OpenEnvBoxContainer<T>::GetData()
{
  return m_data;
}

template <typename T>
void
OpenEnvBoxContainer<T>::Print(std::ostream& where) const
{
  where << "[";
  for (auto i = m_data.begin(); i != m_data.end(); ++i)
  {
    where << std::to_string(*i);
    auto i2 = i;
    i2++;
    if (i2 != m_data.end())
      where << ", ";
  }
  where << "]";
}


class OpenEnvTupleContainer : public OpenEnvDataContainer
{
public:
  OpenEnvTupleContainer ();
  virtual ~OpenEnvTupleContainer ();

  static TypeId GetTypeId ();

  virtual ns3openenv::DataContainer GetDataContainerPbMsg();

  virtual void Print(std::ostream& where) const;
  friend std::ostream& operator<< (std::ostream& os, const Ptr<OpenEnvTupleContainer> container)
  {
    container->Print(os);
    return os;
  }

  bool Add(Ptr<OpenEnvDataContainer> space);
  Ptr<OpenEnvDataContainer> Get(uint32_t idx);

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  std::vector< Ptr<OpenEnvDataContainer> > m_tuple;
};


class OpenEnvDictContainer : public OpenEnvDataContainer
{
public:
  OpenEnvDictContainer ();
  virtual ~OpenEnvDictContainer ();

  static TypeId GetTypeId ();

  virtual ns3openenv::DataContainer GetDataContainerPbMsg();

  virtual void Print(std::ostream& where) const;
  friend std::ostream& operator<< ( std::ostream& os, const Ptr<OpenEnvDictContainer> container)
  {
    container->Print(os);
    return os;
  }

  bool Add(std::string key, Ptr<OpenEnvDataContainer> value);
  Ptr<OpenEnvDataContainer> Get(std::string key);

protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  std::map< std::string, Ptr<OpenEnvDataContainer> > m_dict;
};

} // end of namespace ns3

#endif /* OPENENV_CONTAINER_H */


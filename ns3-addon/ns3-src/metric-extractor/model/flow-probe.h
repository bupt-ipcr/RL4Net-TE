/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-29 15:59
 * @desc: 在原flow-probe基础上增加了记录转发的数据结构
 */
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2009 INESC Porto
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#ifndef FLOW_PROBE_H
#define FLOW_PROBE_H

#include <map>
#include <vector>

#include "ns3/object.h"
#include "ns3/flow-classifier.h"
#include "ns3/nstime.h"

namespace ns3 {

class FlowMonitor;

/// The FlowProbe class is responsible for listening for packet events
/// in a specific point of the simulated space, report those events to
/// the global FlowMonitor, and collect its own flow statistics
/// regarding only the packets that pass through that probe.
class FlowProbe : public Object
{
private:
  /// Defined and not implemented to avoid misuse
  FlowProbe (FlowProbe const &);
  /// Defined and not implemented to avoid misuse
  /// \returns
  FlowProbe& operator= (FlowProbe const &);

protected:
  /// Constructor
  /// \param flowMonitor the FlowMonitor this probe is associated with
  FlowProbe (Ptr<FlowMonitor> flowMonitor);
  virtual void DoDispose (void);

public:
  virtual ~FlowProbe ();

  /// Register this type.
  /// \return The TypeId.
  static TypeId GetTypeId (void);
  
  /// Structure to hold the statistics of a flow
  struct FlowStats
  {
    FlowStats () : delayFromFirstProbeSum (Seconds (0)), bytes (0), packets (0) {}

    /// packetsDropped[reasonCode] => number of dropped packets
    std::vector<uint32_t> packetsDropped;
    /// bytesDropped[reasonCode] => number of dropped bytes
    std::vector<uint64_t> bytesDropped;
    /// divide by 'packets' to get the average delay from the
    /// first (entry) probe up to this one (partial delay)
    Time delayFromFirstProbeSum;
    /// Number of bytes seen of this flow
    uint64_t bytes;
    /// Number of packets seen of this flow
    uint32_t packets;
  };

  /// Container to map FlowId -> FlowStats
  typedef std::map<FlowId, FlowStats> Stats;

  /// 重新定义Stats，保留原内容的情况下增加interface项和nodeId
  // TODO 确认nodeId和probe的index是否一致；若一致，可取消这一项。
  struct RLFlowId
  {
    RLFlowId (FlowId fid, uint32_t nid, uint32_t iid)
        : flowId (fid), nodeId (nid), interface (iid){};
    FlowId flowId;
    uint32_t nodeId;
    uint32_t interface;
    // TODO 重载<运算符
    bool
    operator< (const RLFlowId &r2) const
    {
      if (this->flowId < r2.flowId)
        {
          return true;
        }
      else if (this->flowId == r2.flowId)
        {
          return this->interface < r2.interface;
        }
      else
        {
          return false;
        }
    }
  };
  typedef std::map<RLFlowId, FlowStats> RLStats; //!< RL使用的stats

  /// Add a packet data to the flow stats
  /// \param flowId the flow Identifier
  /// \param packetSize the packet size
  /// \param delayFromFirstProbe packet delay
  void AddPacketStats (FlowId flowId, uint32_t packetSize, Time delayFromFirstProbe);
  /// Add a packet drop data to the flow stats
  /// \param flowId the flow Identifier
  /// \param packetSize the packet size
  /// \param reasonCode reason code for the drop
  void AddPacketDropStats (FlowId flowId, uint32_t packetSize, uint32_t reasonCode);

  /// Get the partial flow statistics stored in this probe.  With this
  /// information you can, for example, find out what is the delay
  /// from the first probe to this one.
  /// \returns the partial flow statistics
  Stats GetStats () const;

  /// Get the partial rl flow statistics stored in this probe.  With this
  /// information you can, for example, find out what is the delay
  /// from the first probe to this one.
  /// \returns the partial flow statistics
  RLStats GetRLStats () const;

  /// Serializes the results to an std::ostream in XML format
  /// \param os the output stream
  /// \param indent number of spaces to use as base indentation level
  /// \param index FlowProbe index
  void SerializeToXmlStream (std::ostream &os, uint16_t indent, uint32_t index) const;

protected:
  Ptr<FlowMonitor> m_flowMonitor; //!< the FlowMonitor instance
  Stats m_stats; //!< The flow stats
  RLStats m_rlstats; //!< rl使用的stats
  
};


} // namespace ns3

#endif /* FLOW_PROBE_H */

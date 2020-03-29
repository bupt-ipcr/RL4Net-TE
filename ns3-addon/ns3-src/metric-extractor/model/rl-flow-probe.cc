/*
 * @author: Jiawei Wu
 * @create time: 2020-03-17 20:52
 * @edit time: 2020-03-29 15:59
 * @desc: 继承改动过的flow-probe，实现记录包转发信息
 */


#include "ns3/rl-flow-probe.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/flow-monitor.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/config.h"
#include "ns3/flow-id-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RLFlowProbe");

//////////////////////////////////////
// RLFlowProbeTag class implementation //
//////////////////////////////////////

/**
 * \ingroup flow-monitor
 *
 * \brief Tag used to allow a fast identification of the packet
 *
 * This tag is added by FlowMonitor when a packet is seen for
 * the first time, and it is then used to classify the packet in
 * the following hops.
 */
class RLFlowProbeTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;
  RLFlowProbeTag ();
  /**
   * \brief Consructor
   * \param flowId the flow identifier
   * \param packetId the packet identifier
   * \param packetSize the packet size
   * \param src packet source address
   * \param dst packet destination address
   */
  RLFlowProbeTag (uint32_t flowId, uint32_t packetId, uint32_t packetSize, Ipv4Address src, Ipv4Address dst);
  /**
   * \brief Set the flow identifier
   * \param flowId the flow identifier
   */
  void SetFlowId (uint32_t flowId);
  /**
   * \brief Set the packet identifier
   * \param packetId the packet identifier
   */
  void SetPacketId (uint32_t packetId);
  /**
   * \brief Set the packet size
   * \param packetSize the packet size
   */
  void SetPacketSize (uint32_t packetSize);
  /**
   * \brief Set the flow identifier
   * \returns the flow identifier
   */
  uint32_t GetFlowId (void) const;
  /**
   * \brief Set the packet identifier
   * \returns the packet identifier
   */
  uint32_t GetPacketId (void) const;
  /**
   * \brief Get the packet size
   * \returns the packet size
   */
  uint32_t GetPacketSize (void) const;
  /**
   * \brief Checks if the addresses stored in tag are matching
   * the arguments.
   *
   * This check is important for IP over IP encapsulation.
   *
   * \param src Source address.
   * \param dst Destination address.
   * \returns True if the addresses are matching.
   */
  bool IsSrcDstValid (Ipv4Address src, Ipv4Address dst) const;
private:
  uint32_t m_flowId;      //!< flow identifier
  uint32_t m_packetId;    //!< packet identifier
  uint32_t m_packetSize;  //!< packet size
  Ipv4Address m_src;      //!< IP source
  Ipv4Address m_dst;      //!< IP destination
};

TypeId 
RLFlowProbeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RLFlowProbeTag")
    .SetParent<Tag> ()
    .SetGroupName ("FlowMonitor")
    .AddConstructor<RLFlowProbeTag> ()
  ;
  return tid;
}
TypeId 
RLFlowProbeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
RLFlowProbeTag::GetSerializedSize (void) const
{
  return 4 + 4 + 4 + 8;
}
void 
RLFlowProbeTag::Serialize (TagBuffer buf) const
{
  buf.WriteU32 (m_flowId);
  buf.WriteU32 (m_packetId);
  buf.WriteU32 (m_packetSize);

  uint8_t tBuf[4];
  m_src.Serialize (tBuf);
  buf.Write (tBuf, 4);
  m_dst.Serialize (tBuf);
  buf.Write (tBuf, 4);
}
void 
RLFlowProbeTag::Deserialize (TagBuffer buf)
{
  m_flowId = buf.ReadU32 ();
  m_packetId = buf.ReadU32 ();
  m_packetSize = buf.ReadU32 ();

  uint8_t tBuf[4];
  buf.Read (tBuf, 4);
  m_src = Ipv4Address::Deserialize (tBuf);
  buf.Read (tBuf, 4);
  m_dst = Ipv4Address::Deserialize (tBuf);
}
void 
RLFlowProbeTag::Print (std::ostream &os) const
{
  os << "FlowId=" << m_flowId;
  os << " PacketId=" << m_packetId;
  os << " PacketSize=" << m_packetSize;
}
RLFlowProbeTag::RLFlowProbeTag ()
  : Tag () 
{
}

RLFlowProbeTag::RLFlowProbeTag (uint32_t flowId, uint32_t packetId, uint32_t packetSize, Ipv4Address src, Ipv4Address dst)
  : Tag (), m_flowId (flowId), m_packetId (packetId), m_packetSize (packetSize), m_src (src), m_dst (dst)
{
}

void
RLFlowProbeTag::SetFlowId (uint32_t id)
{
  m_flowId = id;
}
void
RLFlowProbeTag::SetPacketId (uint32_t id)
{
  m_packetId = id;
}
void
RLFlowProbeTag::SetPacketSize (uint32_t size)
{
  m_packetSize = size;
}
uint32_t
RLFlowProbeTag::GetFlowId (void) const
{
  return m_flowId;
}
uint32_t
RLFlowProbeTag::GetPacketId (void) const
{
  return m_packetId;
} 
uint32_t
RLFlowProbeTag::GetPacketSize (void) const
{
  return m_packetSize;
}
bool
RLFlowProbeTag::IsSrcDstValid (Ipv4Address src, Ipv4Address dst) const
{
  return ((m_src == src) && (m_dst == dst));
}

////////////////////////////////////////
// RLFlowProbe class implementation //
////////////////////////////////////////

RLFlowProbe::RLFlowProbe (Ptr<FlowMonitor> monitor,
                              Ptr<Ipv4FlowClassifier> classifier,
                              Ptr<Node> node)
  : FlowProbe (monitor),
    m_classifier (classifier)
{
  NS_LOG_FUNCTION (this << node->GetId ());
  m_nodeId = node->GetId();
  m_ipv4 = node->GetObject<Ipv4L3Protocol> ();

  if (!m_ipv4->TraceConnectWithoutContext ("SendOutgoing",
                                           MakeCallback (&RLFlowProbe::SendOutgoingLogger, Ptr<RLFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }
  if (!m_ipv4->TraceConnectWithoutContext ("UnicastForward",
                                           MakeCallback (&RLFlowProbe::ForwardLogger, Ptr<RLFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }
  if (!m_ipv4->TraceConnectWithoutContext ("LocalDeliver",
                                           MakeCallback (&RLFlowProbe::ForwardUpLogger, Ptr<RLFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }

  if (!m_ipv4->TraceConnectWithoutContext ("Drop",
                                           MakeCallback (&RLFlowProbe::DropLogger, Ptr<RLFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }

  std::ostringstream qd;
  qd << "/NodeList/" << node->GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/*/Drop";
  Config::ConnectWithoutContext (qd.str (), MakeCallback (&RLFlowProbe::QueueDiscDropLogger, Ptr<RLFlowProbe> (this)));

  // code copied from point-to-point-helper.cc
  std::ostringstream oss;
  oss << "/NodeList/" << node->GetId () << "/DeviceList/*/TxQueue/Drop";
  Config::ConnectWithoutContext (oss.str (), MakeCallback (&RLFlowProbe::QueueDropLogger, Ptr<RLFlowProbe> (this)));
}

RLFlowProbe::~RLFlowProbe ()
{
}

/* static */
TypeId
RLFlowProbe::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RLFlowProbe")
    .SetParent<FlowProbe> ()
    .SetGroupName ("FlowMonitor")
    // No AddConstructor because this class has no default constructor.
    ;

  return tid;
}

void
RLFlowProbe::DoDispose ()
{
  m_ipv4 = 0;
  m_classifier = 0;
  FlowProbe::DoDispose ();
}

void
RLFlowProbe::SendOutgoingLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
{
  FlowId flowId;
  FlowPacketId packetId;

  if (!m_ipv4->IsUnicast(ipHeader.GetDestination ()))
    {
      // we are not prepared to handle broadcast yet
      return;
    }

  RLFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);
  if (found)
    {
      return;
    }

  if (m_classifier->Classify (ipHeader, ipPayload, &flowId, &packetId))
    {
      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportFirstTx ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<"); "
                                     << ipHeader << *ipPayload);
      m_flowMonitor->ReportFirstTx (this, flowId, packetId, size, m_nodeId, interface);

      // tag the packet with the flow id and packet id, so that the packet can be identified even
      // when Ipv4Header is not accessible at some non-IPv4 protocol layer
      RLFlowProbeTag fTag (flowId, packetId, size, ipHeader.GetSource (), ipHeader.GetDestination ());
      ipPayload->AddByteTag (fTag);
    }
}

void
RLFlowProbe::ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
{
  RLFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  if (found)
    {
      if (!ipHeader.IsLastFragment () || ipHeader.GetFragmentOffset () != 0)
        {
          NS_LOG_WARN ("Not counting fragmented packets");
          return;
        }
      if (!fTag.IsSrcDstValid (ipHeader.GetSource (), ipHeader.GetDestination ()))
        {
          NS_LOG_LOGIC ("Not reporting encapsulated packet");
          return;
        }

      FlowId flowId = fTag.GetFlowId ();
      FlowPacketId packetId = fTag.GetPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportForwarding ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<");");
      m_flowMonitor->ReportForwarding (this, flowId, packetId, size, m_nodeId, interface);
    }
}

// 重载report方法
void
RLFlowProbe::AddPacketStats (FlowId flowId, uint32_t nodeId, uint32_t interface, uint32_t packetSize, Time delayFromFirstProbe)
{
  RLFlowId rlFlowId = RLFlowId(flowId, nodeId, interface);
  FlowStats &flow = m_rlstats[rlFlowId];
  flow.delayFromFirstProbeSum += delayFromFirstProbe;
  flow.bytes += packetSize;
  ++flow.packets;
}


void
RLFlowProbe::ForwardUpLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
{
  RLFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  if (found)
    {
      if (!fTag.IsSrcDstValid (ipHeader.GetSource (), ipHeader.GetDestination ()))
        {
          NS_LOG_LOGIC ("Not reporting encapsulated packet");
          return;
        }

      FlowId flowId = fTag.GetFlowId ();
      FlowPacketId packetId = fTag.GetPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportLastRx ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<"); "
                                     << ipHeader << *ipPayload);
      m_flowMonitor->ReportLastRx (this, flowId, packetId, size);
    }
}

void
RLFlowProbe::DropLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload,
                           Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t ifIndex)
{
#if 0
  switch (reason)
    {
    case Ipv4L3Protocol::DROP_NO_ROUTE:
      break;

    case Ipv4L3Protocol::DROP_TTL_EXPIRED:
    case Ipv4L3Protocol::DROP_BAD_CHECKSUM:
      Ipv4Address addri = m_ipv4->GetAddress (ifIndex);
      Ipv4Mask maski = m_ipv4->GetNetworkMask (ifIndex);
      Ipv4Address bcast = addri.GetSubnetDirectedBroadcast (maski);
      if (ipHeader.GetDestination () == bcast) // we don't want broadcast packets
        {
          return;
        }
    }
#endif

  RLFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  if (found)
    {
      FlowId flowId = fTag.GetFlowId ();
      FlowPacketId packetId = fTag.GetPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("Drop ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<", " << reason 
                            << ", destIp=" << ipHeader.GetDestination () << "); "
                            << "HDR: " << ipHeader << " PKT: " << *ipPayload);

      DropReason myReason;


      switch (reason)
        {
        case Ipv4L3Protocol::DROP_TTL_EXPIRED:
          myReason = DROP_TTL_EXPIRE;
          NS_LOG_DEBUG ("DROP_TTL_EXPIRE");
          break;
        case Ipv4L3Protocol::DROP_NO_ROUTE:
          myReason = DROP_NO_ROUTE;
          NS_LOG_DEBUG ("DROP_NO_ROUTE");
          break;
        case Ipv4L3Protocol::DROP_BAD_CHECKSUM:
          myReason = DROP_BAD_CHECKSUM;
          NS_LOG_DEBUG ("DROP_BAD_CHECKSUM");
          break;
        case Ipv4L3Protocol::DROP_INTERFACE_DOWN:
          myReason = DROP_INTERFACE_DOWN;
          NS_LOG_DEBUG ("DROP_INTERFACE_DOWN");
          break;
        case Ipv4L3Protocol::DROP_ROUTE_ERROR:
          myReason = DROP_ROUTE_ERROR;
          NS_LOG_DEBUG ("DROP_ROUTE_ERROR");
          break;
        case Ipv4L3Protocol::DROP_FRAGMENT_TIMEOUT:
          myReason = DROP_FRAGMENT_TIMEOUT;
          NS_LOG_DEBUG ("DROP_FRAGMENT_TIMEOUT");
          break;

        default:
          myReason = DROP_INVALID_REASON;
          NS_FATAL_ERROR ("Unexpected drop reason code " << reason);
        }

      m_flowMonitor->ReportDrop (this, flowId, packetId, size, myReason);
    }
}

void 
RLFlowProbe::QueueDropLogger (Ptr<const Packet> ipPayload)
{
  RLFlowProbeTag fTag;
  bool tagFound = ipPayload->FindFirstMatchingByteTag (fTag);

  if (!tagFound)
    {
      return;
    }

  FlowId flowId = fTag.GetFlowId ();
  FlowPacketId packetId = fTag.GetPacketId ();
  uint32_t size = fTag.GetPacketSize ();

  NS_LOG_DEBUG ("Drop ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<", " << DROP_QUEUE 
                        << "); ");

  m_flowMonitor->ReportDrop (this, flowId, packetId, size, DROP_QUEUE);
}

void
RLFlowProbe::QueueDiscDropLogger (Ptr<const QueueDiscItem> item)
{
  RLFlowProbeTag fTag;
  bool tagFound = item->GetPacket ()->FindFirstMatchingByteTag (fTag);

  if (!tagFound)
    {
      return;
    }

  FlowId flowId = fTag.GetFlowId ();
  FlowPacketId packetId = fTag.GetPacketId ();
  uint32_t size = fTag.GetPacketSize ();

  NS_LOG_DEBUG ("Drop ("<<this<<", "<<flowId<<", "<<packetId<<", "<<size<<", " << DROP_QUEUE_DISC
                        << "); ");

  m_flowMonitor->ReportDrop (this, flowId, packetId, size, DROP_QUEUE_DISC);
}

} // namespace ns3



#include "rtmhr.h"

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/tcp-header.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"

#define RTMHR_PORT 654

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RtMhr");

namespace rtmhr
{

/**
 * \ingroup rtmhr
 * \brief RT-MHR message header
 */
class RtMhrHeader : public Header
{
  public:
    RtMhrHeader(MessageType type = RTMHR_HELLO,
                uint8_t hopCount = 0,
                uint32_t requestId = 0,
                Ipv4Address dst = Ipv4Address(),
                Ipv4Address origin = Ipv4Address(),
                double linkQuality = 0.0,
                double delay = 0.0,
                double mobility = 0.0);

    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const;
    virtual void Print(std::ostream& os) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);
    virtual uint32_t GetSerializedSize() const;

    // Getters and Setters
    void SetMessageType(MessageType type)
    {
        m_type = type;
    }

    MessageType GetMessageType() const
    {
        return m_type;
    }

    void SetHopCount(uint8_t hopCount)
    {
        m_hopCount = hopCount;
    }

    uint8_t GetHopCount() const
    {
        return m_hopCount;
    }

    void SetRequestId(uint32_t requestId)
    {
        m_requestId = requestId;
    }

    uint32_t GetRequestId() const
    {
        return m_requestId;
    }

    void SetDestination(Ipv4Address dst)
    {
        m_dst = dst;
    }

    Ipv4Address GetDestination() const
    {
        return m_dst;
    }

    void SetOrigin(Ipv4Address origin)
    {
        m_origin = origin;
    }

    Ipv4Address GetOrigin() const
    {
        return m_origin;
    }

    void SetLinkQuality(double quality)
    {
        m_linkQuality = quality;
    }

    double GetLinkQuality() const
    {
        return m_linkQuality;
    }

    void SetDelay(double delay)
    {
        m_delay = delay;
    }

    double GetDelay() const
    {
        return m_delay;
    }

    void SetMobility(double mobility)
    {
        m_mobility = mobility;
    }

    double GetMobility() const
    {
        return m_mobility;
    }

    void SetSequenceNumber(uint32_t seqNum)
    {
        m_sequenceNumber = seqNum;
    }

    uint32_t GetSequenceNumber() const
    {
        return m_sequenceNumber;
    }

  private:
    MessageType m_type;        ///< Message type
    uint8_t m_hopCount;        ///< Hop count
    uint32_t m_requestId;      ///< Request ID
    Ipv4Address m_dst;         ///< Destination address
    Ipv4Address m_origin;      ///< Originator address
    double m_linkQuality;      ///< Link quality metric
    double m_delay;            ///< Delay metric
    double m_mobility;         ///< Mobility metric
    uint32_t m_sequenceNumber; ///< Sequence number
};

RtMhrHeader::RtMhrHeader(MessageType type,
                         uint8_t hopCount,
                         uint32_t requestId,
                         Ipv4Address dst,
                         Ipv4Address origin,
                         double linkQuality,
                         double delay,
                         double mobility)
    : m_type(type),
      m_hopCount(hopCount),
      m_requestId(requestId),
      m_dst(dst),
      m_origin(origin),
      m_linkQuality(linkQuality),
      m_delay(delay),
      m_mobility(mobility),
      m_sequenceNumber(0)
{
}

TypeId
RtMhrHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::rtmhr::RtMhrHeader").SetParent<Header>().AddConstructor<RtMhrHeader>();
    return tid;
}

TypeId
RtMhrHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RtMhrHeader::Print(std::ostream& os) const
{
    os << "RtMhrHeader: type=" << m_type << " hopCount=" << (uint32_t)m_hopCount
       << " requestId=" << m_requestId << " dst=" << m_dst << " origin=" << m_origin;
}

uint32_t
RtMhrHeader::GetSerializedSize() const
{
    return 1 + 1 + 4 + 4 + 4 + 8 + 8 + 8 + 4; // type + hopCount + requestId + dst + origin +
                                              // linkQuality + delay + mobility + seqNum
}

void
RtMhrHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(static_cast<uint8_t>(m_type));
    start.WriteU8(m_hopCount);
    start.WriteHtonU32(m_requestId);
    start.WriteHtonU32(m_dst.Get());
    start.WriteHtonU32(m_origin.Get());
    start.WriteU64(m_linkQuality);
    start.WriteU64(m_delay);
    start.WriteU64(m_mobility);
    start.WriteHtonU32(m_sequenceNumber);
}

uint32_t
RtMhrHeader::Deserialize(Buffer::Iterator start)
{
    m_type = static_cast<MessageType>(start.ReadU8());
    m_hopCount = start.ReadU8();
    m_requestId = start.ReadNtohU32();
    m_dst.Set(start.ReadNtohU32());
    m_origin.Set(start.ReadNtohU32());
    m_linkQuality = start.ReadU64();
    m_delay = start.ReadU64();
    m_mobility = start.ReadU64();
    m_sequenceNumber = start.ReadNtohU32();

    return GetSerializedSize();
}

} // namespace rtmhr

// CrossLayerMetric implementation
CrossLayerMetric::CrossLayerMetric()
    : linkQuality(0.0),
      queuingDelay(0.0),
      mobilityMetric(0.0),
      hopCount(0),
      timestamp(0.0)
{
}

double
CrossLayerMetric::GetCompositeMetric() const
{
    // Composite metric calculation with weights
    double composite = (0.3 * linkQuality) + (0.25 * (1.0 / (queuingDelay + 0.001))) +
                       (0.25 * (1.0 / (mobilityMetric + 0.001))) + (0.2 * (1.0 / (hopCount + 1)));
    return composite;
}

// NeighborEntry implementation
NeighborEntry::NeighborEntry(Ipv4Address addr)
    : address(addr),
      lastSeen(Simulator::Now()),
      linkQuality(1.0),
      interface(0),
      validTime(Simulator::Now() + Seconds(30))
{
}

bool
NeighborEntry::IsExpired() const
{
    return Simulator::Now() > validTime;
}

// RouteEntry implementation
RouteEntry::RouteEntry(Ipv4Address dest)
    : destination(dest),
      interface(0),
      hopCount(0),
      sequenceNumber(0),
      validTime(Simulator::Now() + Seconds(30)),
      isPrimary(false)
{
}

bool
RouteEntry::IsExpired() const
{
    return Simulator::Now() > validTime;
}

// RT-MHR Protocol Implementation
NS_OBJECT_ENSURE_REGISTERED(RtMhr);

TypeId
RtMhr::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RtMhr")
                            .SetParent<Ipv4RoutingProtocol>()
                            .AddConstructor<RtMhr>()
                            .AddAttribute("HelloInterval",
                                          "HELLO messages emission interval.",
                                          TimeValue(Seconds(1)),
                                          MakeTimeAccessor(&RtMhr::m_helloInterval),
                                          MakeTimeChecker())
                            .AddAttribute("NeighborTimeout",
                                          "Validity time for neighbors.",
                                          TimeValue(Seconds(3)),
                                          MakeTimeAccessor(&RtMhr::m_neighborTimeout),
                                          MakeTimeChecker())
                            .AddAttribute("RouteTimeout",
                                          "Validity time for routes.",
                                          TimeValue(Seconds(30)),
                                          MakeTimeAccessor(&RtMhr::m_routeTimeout),
                                          MakeTimeChecker())
                            .AddAttribute("FastLocalRepair",
                                          "Enable fast local repair mechanism.",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&RtMhr::m_fastLocalRepair),
                                          MakeBooleanChecker())
                            .AddAttribute("LinkQualityWeight",
                                          "Weight for link quality in metric calculation.",
                                          DoubleValue(0.3),
                                          MakeDoubleAccessor(&RtMhr::m_linkQualityWeight),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("DelayWeight",
                                          "Weight for delay in metric calculation.",
                                          DoubleValue(0.25),
                                          MakeDoubleAccessor(&RtMhr::m_delayWeight),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("MobilityWeight",
                                          "Weight for mobility in metric calculation.",
                                          DoubleValue(0.25),
                                          MakeDoubleAccessor(&RtMhr::m_mobilityWeight),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("HopCountWeight",
                                          "Weight for hop count in metric calculation.",
                                          DoubleValue(0.2),
                                          MakeDoubleAccessor(&RtMhr::m_hopCountWeight),
                                          MakeDoubleChecker<double>())
                            .AddTraceSource("Tx",
                                            "Send RT-MHR packet.",
                                            MakeTraceSourceAccessor(&RtMhr::m_txTrace),
                                            "ns3::Packet::TracedCallback")
                            .AddTraceSource("Rx",
                                            "Receive RT-MHR packet.",
                                            MakeTraceSourceAccessor(&RtMhr::m_rxTrace),
                                            "ns3::Packet::TracedCallback");
    return tid;
}

RtMhr::RtMhr()
    : m_helloTimer(Timer::CANCEL_ON_DESTROY),
      m_probeTimer(Timer::CANCEL_ON_DESTROY),
      m_purgeTimer(Timer::CANCEL_ON_DESTROY),
      m_helloInterval(Seconds(1)),
      m_neighborTimeout(Seconds(3)),
      m_routeTimeout(Seconds(30)),
      m_probeInterval(Seconds(5)),
      m_fastLocalRepair(true),
      m_requestId(0),
      m_sequenceNumber(0),
      m_linkQualityWeight(0.3),
      m_delayWeight(0.25),
      m_mobilityWeight(0.25),
      m_hopCountWeight(0.2)
{
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

RtMhr::~RtMhr()
{
}

void
RtMhr::DoDispose()
{
    m_ipv4 = 0;
    for (auto iter = m_socketAddresses.begin(); iter != m_socketAddresses.end(); iter++)
    {
        iter->first->Close();
    }
    m_socketAddresses.clear();
    Ipv4RoutingProtocol::DoDispose();
}

void
RtMhr::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_ASSERT(ipv4);
    NS_ASSERT(!m_ipv4);
    m_ipv4 = ipv4;

    // Start protocol after delay
    Simulator::ScheduleWithContext(m_ipv4->GetObject<Node>()->GetId(),
                                   Seconds(m_uniformRandomVariable->GetValue(0, 1)),
                                   &RtMhr::Start,
                                   this);
}

void
RtMhr::Start()
{
    NS_LOG_FUNCTION(this);

    // Set up control socket for RT-MHR messages
    m_recvSocket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), RTMHR_PORT);
    m_recvSocket->Bind(local);
    m_recvSocket->SetRecvCallback(MakeCallback(&RtMhr::RecvRtMhr, this));
    m_recvSocket->SetAllowBroadcast(true);

    // Set up hello timer
    m_helloTimer.SetFunction(&RtMhr::HelloTimerExpire, this);
    m_helloTimer.Schedule(m_helloInterval);

    // Set up probe timer
    m_probeTimer.SetFunction(&RtMhr::ProbeTimerExpire, this);
    m_probeTimer.Schedule(m_probeInterval);

    // Set up purge timer
    m_purgeTimer.SetFunction(&RtMhr::PurgeTimerExpire, this);
    m_purgeTimer.Schedule(Seconds(1));
}

void
RtMhr::Stop()
{
    NS_LOG_FUNCTION(this);
    m_helloTimer.Cancel();
    m_probeTimer.Cancel();
    m_purgeTimer.Cancel();

    if (m_recvSocket)
    {
        m_recvSocket->Close();
        m_recvSocket = 0;
    }

    for (auto& timer : m_rreqTimers)
    {
        timer.second.Cancel();
    }
    m_rreqTimers.clear();
}

Ptr<Ipv4Route>
RtMhr::RouteOutput(Ptr<Packet> p,
                   const Ipv4Header& header,
                   Ptr<NetDevice> oif,
                   Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << header << (oif ? oif->GetIfIndex() : 0));

    if (!p)
    {
        NS_LOG_DEBUG("Packet is 0");
        return LoopbackRoute(header, oif);
    }

    if (m_socketAddresses.empty())
    {
        sockerr = Socket::ERROR_NOROUTETOHOST;
        NS_LOG_LOGIC("No RT-MHR interfaces");
        return Ptr<Ipv4Route>();
    }

    Ipv4Address dst = header.GetDestination();
    NS_LOG_DEBUG("Looking for route to " << dst);

    // Check if destination is in routing table
    auto routeIter = m_routeTable.find(dst);
    if (routeIter != m_routeTable.end() && !routeIter->second.IsExpired())
    {
        Ptr<Ipv4Route> route = Create<Ipv4Route>();
        route->SetDestination(dst);
        route->SetGateway(routeIter->second.nextHop);
        route->SetSource(GetAddressForInterface(routeIter->second.interface));
        route->SetOutputDevice(GetNetDeviceForInterface(routeIter->second.interface));
        sockerr = Socket::ERROR_NOTERROR;
        NS_LOG_DEBUG("Found route to " << dst << " via " << routeIter->second.nextHop);
        return route;
    }

    // No route found, try to create a direct route for same subnet
    NS_LOG_DEBUG("No route found for " << dst << ", checking for direct connectivity");

    // Check if destination is on same subnet as any of our interfaces
    for (auto& addr : m_socketAddresses)
    {
        Ipv4InterfaceAddress iaddr = addr.second;
        if (dst.IsSubnetDirectedBroadcast(iaddr.GetMask()) ||
            dst.GetSubnetDirectedBroadcast(iaddr.GetMask()) ==
                iaddr.GetLocal().GetSubnetDirectedBroadcast(iaddr.GetMask()))
        {
            // Destination is on same subnet, create direct route
            RouteEntry entry;
            entry.destination = dst;
            entry.nextHop = dst; // Direct route
            entry.interface = GetInterfaceForDevice(addr.first->GetBoundNetDevice());
            entry.metric = CrossLayerMetric();                // Default metric
            entry.validTime = Simulator::Now() + Seconds(30); // 30 second lifetime

            m_routeTable[dst] = entry;

            // Create and return the route
            Ptr<Ipv4Route> route = Create<Ipv4Route>();
            route->SetDestination(dst);
            route->SetGateway(dst); // Direct route
            route->SetSource(iaddr.GetLocal());
            route->SetOutputDevice(addr.first->GetBoundNetDevice());
            sockerr = Socket::ERROR_NOTERROR;

            NS_LOG_DEBUG("Created direct route to " << dst);
            return route;
        }
    }

    // If we can't create a direct route, initiate route discovery
    SendRouteRequest(dst);

    sockerr = Socket::ERROR_NOROUTETOHOST;
    return Ptr<Ipv4Route>();
}

bool
RtMhr::RouteInput(Ptr<const Packet> p,
                  const Ipv4Header& header,
                  Ptr<const NetDevice> idev,
                  const UnicastForwardCallback& ucb,
                  const MulticastForwardCallback& mcb,
                  const LocalDeliverCallback& lcb,
                  const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p->GetUid() << header.GetDestination() << idev->GetAddress());

    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No RT-MHR interfaces");
        return false;
    }

    NS_ASSERT(m_ipv4);
    NS_ASSERT(p);

    int32_t iif = m_ipv4->GetInterfaceForDevice(idev);
    NS_ASSERT(iif >= 0);

    Ipv4Address dst = header.GetDestination();
    Ipv4Address origin = header.GetSource();

    // Check if packet is for local delivery
    if (m_ipv4->IsDestinationAddress(dst, iif))
    {
        if (dst.IsLocalhost())
        {
            lcb(p, header, iif);
            return true;
        }

        // Check if it's an RT-MHR control packet
        if (header.GetProtocol() == 17) // UDP protocol number
        {
            Ptr<Packet> packet = p->Copy();
            UdpHeader udpHeader;
            packet->RemoveHeader(udpHeader);

            if (udpHeader.GetDestinationPort() == RTMHR_PORT)
            {
                // It's an RT-MHR control packet, process it
                Ptr<Socket> fakeSocket = nullptr;
                RecvRtMhr(fakeSocket);
                return true;
            }
        }

        lcb(p, header, iif);
        return true;
    }

    // Packet needs to be forwarded
    return ForwardPacketTo(p, header, ucb, ecb);
}

bool
RtMhr::ForwardPacketTo(Ptr<const Packet> p,
                       const Ipv4Header& header,
                       const UnicastForwardCallback& ucb,
                       const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p->GetUid() << header.GetDestination());

    Ipv4Address dst = header.GetDestination();

    // Look for route in routing table
    auto routeIter = m_routeTable.find(dst);
    if (routeIter != m_routeTable.end() && !routeIter->second.IsExpired())
    {
        Ptr<Ipv4Route> route = Create<Ipv4Route>();
        route->SetDestination(dst);
        route->SetGateway(routeIter->second.nextHop);
        route->SetSource(GetAddressForInterface(routeIter->second.interface));
        route->SetOutputDevice(GetNetDeviceForInterface(routeIter->second.interface));

        ucb(route, p, header);
        return true;
    }

    // No route found
    NS_LOG_DEBUG("No route found for " << dst);
    return false;
}

Ptr<NetDevice>
RtMhr::GetNetDeviceFromContext() const
{
    // Helper method to get current net device from context
    // This is a simplified implementation
    return m_ipv4->GetNetDevice(0);
}

void
RtMhr::NotifyTxOk(const WifiMacHeader& hdr)
{
    // Callback for successful transmissions - used for link quality estimation
    NS_LOG_FUNCTION(this);
}

Ptr<Ipv4Route>
RtMhr::LoopbackRoute(const Ipv4Header& hdr, Ptr<NetDevice> oif) const
{
    NS_LOG_FUNCTION(this << hdr << oif);
    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(hdr.GetDestination());

    // Source address selection
    uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice(oif) : -1);

    // Single interface simple case
    if (m_ipv4->GetNInterfaces() == 1)
    {
        Ipv4InterfaceAddress addr = m_ipv4->GetAddress(0, 0);
        rt->SetSource(addr.GetLocal());
        rt->SetGateway(Ipv4Address("127.0.0.1"));
        rt->SetOutputDevice(m_ipv4->GetNetDevice(0));
        return rt;
    }

    // Otherwise, we select the first available address for now
    if (iif >= 0)
    {
        Ipv4InterfaceAddress addr = m_ipv4->GetAddress(iif, 0);
        rt->SetSource(addr.GetLocal());
    }
    else
    {
        rt->SetSource(m_ipv4->GetAddress(0, 0).GetLocal());
    }

    rt->SetGateway(Ipv4Address("127.0.0.1"));
    rt->SetOutputDevice(oif ? oif : m_ipv4->GetNetDevice(0));
    return rt;
}

void
RtMhr::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << m_ipv4->GetAddress(i, 0).GetLocal());

    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    if (l3->GetNAddresses(i) > 1)
    {
        NS_LOG_WARN("RT-MHR does not work with more than one address per each interface.");
    }

    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
    {
        return;
    }

    // Create a socket for this interface
    Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket);
    socket->SetRecvCallback(MakeCallback(&RtMhr::RecvRtMhr, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), RTMHR_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);

    m_socketAddresses.insert(std::make_pair(socket, iface));

    // Allow neighbor layer access
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(i);
    if (dev->GetInstanceTypeId() == WifiNetDevice::GetTypeId())
    {
        Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(dev);
        if (wifi)
        {
            Ptr<WifiMac> mac = wifi->GetMac();
            if (mac)
            {
                mac->TraceConnectWithoutContext("TxOkHeader",
                                                MakeCallback(&RtMhr::NotifyTxOk, this));
            }
        }
    }
}

void
RtMhr::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << m_ipv4->GetAddress(i, 0).GetLocal());

    // Close socket and remove from map
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    NS_ASSERT(socket);
    socket->Close();
    m_socketAddresses.erase(socket);

    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No RT-MHR interfaces");
        Stop();
        m_routeTable.clear();
        m_neighborTable.clear();
        return;
    }
}

void
RtMhr::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << " interface " << interface << " address " << address);
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    if (!l3->IsUp(interface))
    {
        return;
    }

    if (l3->GetNAddresses(interface) == 1)
    {
        Ipv4InterfaceAddress iface = l3->GetAddress(interface, 0);
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(iface);
        if (!socket)
        {
            if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
            {
                return;
            }

            Ptr<Socket> socket =
                Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RtMhr::RecvRtMhr, this));
            socket->BindToNetDevice(l3->GetNetDevice(interface));
            socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), RTMHR_PORT));
            socket->SetAllowBroadcast(true);
            m_socketAddresses.insert(std::make_pair(socket, iface));
        }
    }
    else
    {
        NS_LOG_WARN("RT-MHR does not work with more than one address per each interface. Ignore "
                    "added address");
    }
}

void
RtMhr::NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
    if (socket)
    {
        socket->Close();
        m_socketAddresses.erase(socket);

        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
        if (l3->GetNAddresses(i))
        {
            Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
            // Create a socket for the remaining address
            socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RtMhr::RecvRtMhr, this));
            socket->BindToNetDevice(l3->GetNetDevice(i));
            socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), RTMHR_PORT));
            socket->SetAllowBroadcast(true);
            m_socketAddresses.insert(std::make_pair(socket, iface));
        }

        if (m_socketAddresses.empty())
        {
            NS_LOG_LOGIC("No RT-MHR interfaces");
            Stop();
            m_routeTable.clear();
            m_neighborTable.clear();
            return;
        }
    }
    else
    {
        NS_LOG_WARN("Remove address not participating in RT-MHR operation");
    }
}

void
RtMhr::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    *stream->GetStream() << "Node: " << m_ipv4->GetObject<Node>()->GetId()
                         << ", Time: " << Now().As(unit)
                         << ", Local time: " << GetObject<Node>()->GetLocalTime().As(unit)
                         << ", RT-MHR Routing table" << std::endl;

    *stream->GetStream() << "Destination\tNext Hop\tInterface\tMetric\tExpiry" << std::endl;

    for (auto iter = m_routeTable.begin(); iter != m_routeTable.end(); ++iter)
    {
        std::ostringstream dest, gw, interface, metric, expiry;
        dest << iter->second.destination;
        gw << iter->second.nextHop;
        interface << iter->second.interface;
        metric << iter->second.metric.GetCompositeMetric();
        expiry << std::max(Seconds(0), iter->second.validTime - Now()).As(unit);

        *stream->GetStream() << dest.str() << "\t" << gw.str() << "\t" << interface.str() << "\t"
                             << metric.str() << "\t" << expiry.str() << std::endl;
    }
    *stream->GetStream() << std::endl;
}

int64_t
RtMhr::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uniformRandomVariable->SetStream(stream);
    return 1;
}

} // namespace ns3

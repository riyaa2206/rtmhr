#ifndef RTMHR_H
#define RTMHR_H

#include "ns3/callback.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/queue.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"

#include <list>
#include <map>
#include <set>
#include <vector>

/**
 * \defgroup rtmhr Real-Time Multi-Hop Routing Protocol
 * \brief RT-MHR: Optimized Multi-Hop Wireless Routing Protocol for Real-Time Data Delivery
 */

namespace ns3
{

/**
 * \ingroup rtmhr
 * \brief RT-MHR Message Types
 */
enum MessageType
{
    RTMHR_RREQ = 1,  ///< Route Request
    RTMHR_RREP = 2,  ///< Route Reply
    RTMHR_RERR = 3,  ///< Route Error
    RTMHR_HELLO = 4, ///< Hello message for neighbor discovery
    RTMHR_PROBE = 5, ///< Link quality probe
    RTMHR_PREP = 6   ///< Path Repair message
};

/**
 * \ingroup rtmhr
 * \brief Traffic Priority Levels
 */
enum TrafficPriority
{
    HIGH_PRIORITY = 1,   ///< High priority traffic (real-time)
    MEDIUM_PRIORITY = 2, ///< Medium priority traffic
    NORMAL_PRIORITY = 3  ///< Normal priority traffic
};

/**
 * \ingroup rtmhr
 * \brief Cross-layer Route Metric structure
 */
struct CrossLayerMetric
{
    double linkQuality;    ///< Link quality (0-1)
    double queuingDelay;   ///< Queuing delay in seconds
    double mobilityMetric; ///< Mobility prediction metric
    uint32_t hopCount;     ///< Number of hops
    double timestamp;      ///< Last update timestamp

    /**
     * \brief Calculate composite metric
     * \return Combined metric value
     */
    double GetCompositeMetric() const;

    /**
     * \brief Default constructor
     */
    CrossLayerMetric();
};

/**
 * \ingroup rtmhr
 * \brief Neighbor Table Entry
 */
struct NeighborEntry
{
    Ipv4Address address;
    Time lastSeen;
    double linkQuality;
    uint32_t interface;
    CrossLayerMetric metric;
    Time validTime;

    NeighborEntry() = default;
    NeighborEntry(Ipv4Address addr);
    bool IsExpired() const;
    void UpdateLinkQuality(double quality);
};

/**
 * \ingroup rtmhr
 * \brief Route Table Entry with multi-path support
 */
struct RouteEntry
{
    Ipv4Address destination;
    Ipv4Address nextHop;
    Ipv4Address gateway;
    uint32_t interface;
    uint32_t hopCount;
    uint32_t sequenceNumber;
    Time validTime;
    bool isPrimary;
    CrossLayerMetric metric;
    std::vector<Ipv4Address> backupPaths;

    RouteEntry() = default;
    RouteEntry(Ipv4Address dest);
    bool IsExpired() const;
    void SetExpired();
    void UpdateMetric(const CrossLayerMetric& newMetric);
};

/**
 * \ingroup rtmhr
 * \brief RT-MHR Routing Protocol implementation
 */
class RtMhr : public Ipv4RoutingProtocol
{
  public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor
     */
    RtMhr();

    /**
     * \brief Destructor
     */
    virtual ~RtMhr();

    // Inherited from Ipv4RoutingProtocol
    virtual Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                                       const Ipv4Header& header,
                                       Ptr<NetDevice> oif,
                                       Socket::SocketErrno& sockerr) override;
    virtual bool RouteInput(Ptr<const Packet> p,
                            const Ipv4Header& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb) override;
    virtual void NotifyInterfaceUp(uint32_t interface) override;
    virtual void NotifyInterfaceDown(uint32_t interface) override;
    virtual void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    virtual void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    virtual void SetIpv4(Ptr<Ipv4> ipv4) override;
    virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                                   Time::Unit unit = Time::S) const override;

    /**
     * \brief Assign a fixed random variable stream number to the random variables used by this
     * model
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    // Protocol Configuration
    /**
     * \brief Set hello interval
     * \param helloInterval Hello message interval
     */
    void SetHelloInterval(Time helloInterval)
    {
        m_helloInterval = helloInterval;
    }

    /**
     * \brief Get hello interval
     * \return Hello message interval
     */
    Time GetHelloInterval() const
    {
        return m_helloInterval;
    }

    /**
     * \brief Set neighbor timeout
     * \param neighborTimeout Neighbor timeout
     */
    void SetNeighborTimeout(Time neighborTimeout)
    {
        m_neighborTimeout = neighborTimeout;
    }

    /**
     * \brief Set route timeout
     * \param routeTimeout Route timeout
     */
    void SetRouteTimeout(Time routeTimeout)
    {
        m_routeTimeout = routeTimeout;
    }

    /**
     * \brief Enable/disable Fast Local Repair
     * \param enable Enable flag
     */
    void SetFastLocalRepair(bool enable)
    {
        m_fastLocalRepair = enable;
    }

  protected:
    virtual void DoDispose() override;

  private:
    // Core Protocol Functions
    void Start();
    void Stop();

    // Route Discovery
    void RouteRequestTimerExpire(Ipv4Address dst);
    void SendRouteRequest(Ipv4Address destination);
    void RecvRouteRequest(Ptr<Packet> packet, Ipv4Address src, Ipv4Address dst);
    void SendRouteReply(Ipv4Address destination, Ipv4Address source, Ipv4Address nextHop);
    void RecvRouteReply(Ptr<Packet> packet, Ipv4Address src, Ipv4Address dst);

    // Route Maintenance
    void SendRouteError(Ipv4Address destination, Ipv4Address unreachable);
    void RecvRouteError(Ptr<Packet> packet, Ipv4Address src);
    void PerformFastLocalRepair(Ipv4Address destination, Ipv4Address failedNextHop);

    // Neighbor Management
    void SendHello();
    void RecvHello(Ptr<Packet> packet, Ipv4Address src);
    void UpdateNeighborTable(Ipv4Address neighbor, const CrossLayerMetric& metric);
    void PurgeNeighborTable();

    // Link Quality Monitoring
    void SendProbe(Ipv4Address neighbor);
    void RecvProbe(Ptr<Packet> packet, Ipv4Address src);
    void UpdateLinkQuality(Ipv4Address neighbor, double quality);
    CrossLayerMetric CalculateCrossLayerMetric(Ipv4Address neighbor);

    // Mobility Prediction
    double PredictMobilityMetric(Ipv4Address neighbor);
    void UpdateMobilityInfo(Ipv4Address neighbor, Vector position, Vector velocity);

    // Priority Queuing
    void ForwardPacket(Ptr<const Packet> packet,
                       const Ipv4Header& header,
                       Ipv4Address nextHop,
                       uint32_t interface);
    TrafficPriority ClassifyTraffic(Ptr<const Packet> packet, const Ipv4Header& header);

    // Utility Functions
    Ipv4Address GetNextHopForDestination(Ipv4Address destination);
    bool IsMyOwnAddress(Ipv4Address src);
    Ipv4Address GetLocalAddress() const;
    Ptr<NetDevice> GetNetDeviceForInterface(uint32_t interface) const;
    Ipv4Address GetAddressForInterface(uint32_t interface) const;
    uint32_t GetInterfaceForDevice(Ptr<NetDevice> dev) const;
    Ptr<NetDevice> GetNetDeviceFromContext() const;
    Ptr<Ipv4Route> LoopbackRoute(const Ipv4Header& hdr, Ptr<NetDevice> oif) const;

    // Forwarding
    bool ForwardPacketTo(Ptr<const Packet> p,
                         const Ipv4Header& header,
                         const UnicastForwardCallback& ucb,
                         const ErrorCallback& ecb); // Socket handling
    void RecvRtMhr(Ptr<Socket> socket);
    Ptr<Socket> FindSocketWithInterfaceAddress(Ipv4InterfaceAddress iface) const;

    // WiFi MAC layer callbacks
    void NotifyTxOk(const WifiMacHeader& hdr);

    // Timer callbacks
    void HelloTimerExpire();
    void ProbeTimerExpire();
    void PurgeTimerExpire();

    // Member Variables
    Ptr<Ipv4> m_ipv4;                                              ///< IPv4 object
    std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses; ///< Socket to interface map
    Ptr<Socket> m_recvSocket; ///< Socket for receiving RT-MHR messages

    // Routing Tables
    std::map<Ipv4Address, RouteEntry> m_routeTable;       ///< Main routing table
    std::map<Ipv4Address, NeighborEntry> m_neighborTable; ///< Neighbor table

    // Timers
    Timer m_helloTimer;                        ///< Hello timer
    Timer m_probeTimer;                        ///< Probe timer
    Timer m_purgeTimer;                        ///< Purge timer
    std::map<Ipv4Address, Timer> m_rreqTimers; ///< RREQ timers

    // Configuration Parameters
    Time m_helloInterval;   ///< Hello interval
    Time m_neighborTimeout; ///< Neighbor timeout
    Time m_routeTimeout;    ///< Route timeout
    Time m_probeInterval;   ///< Probe interval
    bool m_fastLocalRepair; ///< Fast local repair flag

    // Protocol State
    uint32_t m_requestId;                                      ///< Request ID counter
    uint32_t m_sequenceNumber;                                 ///< Sequence number
    std::set<std::pair<Ipv4Address, uint32_t>> m_seenRequests; ///< Seen requests

    // Cross-layer parameters
    double m_linkQualityWeight; ///< Link quality weight in CRM
    double m_delayWeight;       ///< Delay weight in CRM
    double m_mobilityWeight;    ///< Mobility weight in CRM
    double m_hopCountWeight;    ///< Hop count weight in CRM

    // Random number generation
    Ptr<UniformRandomVariable> m_uniformRandomVariable; ///< Uniform random variable

    // Traced callbacks
    TracedCallback<Ptr<const Packet>> m_txTrace; ///< TX trace
    TracedCallback<Ptr<const Packet>> m_rxTrace; ///< RX trace
};

} // namespace ns3

#endif /* RTMHR_H */

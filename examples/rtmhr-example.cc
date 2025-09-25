#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/rtmhr-helper.h"
#include "ns3/wifi-module.h"

/**
 * \file
 * \ingroup rtmhr
 * \brief RT-MHR protocol example simulation
 *
 * This example demonstrates the RT-MHR routing protocol in a wireless ad-hoc network.
 * It creates a simple topology with mobile nodes and shows how RT-MHR handles
 * route discovery, maintenance, and fast local repair.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RtMhrExample");

/**
 * \brief Simple RT-MHR example with mobile nodes
 *
 * This example:
 * - Creates a mobile ad-hoc network with RT-MHR routing
 * - Sets up UDP traffic between nodes
 * - Demonstrates protocol behavior under mobility
 * - Collects performance metrics
 */
class RtMhrExample
{
  public:
    RtMhrExample();
    void Run();
    void CreateNodes();
    void CreateDevices();
    void InstallInternetStack();
    void InstallApplications();
    void SetupMobility();
    void SetupTracing();

  private:
    uint32_t m_nNodes;       ///< Number of nodes
    double m_totalTime;      ///< Total simulation time
    double m_nodeSpeed;      ///< Maximum node speed
    std::string m_phyMode;   ///< PHY mode
    uint32_t m_nodeDistance; ///< Initial distance between nodes

    NodeContainer m_nodes;               ///< Container of nodes
    NetDeviceContainer m_devices;        ///< Container of devices
    Ipv4InterfaceContainer m_interfaces; ///< Container of interfaces
    ApplicationContainer m_clientApps;   ///< Client applications
    ApplicationContainer m_serverApps;   ///< Server applications

    Ptr<FlowMonitor> m_flowMonitor; ///< Flow monitor
};

RtMhrExample::RtMhrExample()
    : m_nNodes(10),
      m_totalTime(200.0),
      m_nodeSpeed(10.0),
      m_phyMode("DsssRate1Mbps"),
      m_nodeDistance(50)
{
}

void
RtMhrExample::CreateNodes()
{
    NS_LOG_INFO("Creating " << m_nNodes << " nodes");
    m_nodes.Create(m_nNodes);
}

void
RtMhrExample::CreateDevices()
{
    NS_LOG_INFO("Setting up WiFi devices");

    // WiFi configuration
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    // Set the PHY parameters
    wifiPhy.Set("TxPowerStart", DoubleValue(33));
    wifiPhy.Set("TxPowerEnd", DoubleValue(33));
    wifiPhy.Set("TxGain", DoubleValue(0));
    wifiPhy.Set("RxGain", DoubleValue(0));
    wifiPhy.Set("RxSensitivity", DoubleValue(-96.0));
    wifiPhy.Set("CcaEdThreshold", DoubleValue(-79.0));

    // MAC configuration
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    // Set remote station manager
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(m_phyMode),
                                 "ControlMode",
                                 StringValue(m_phyMode));

    m_devices = wifi.Install(wifiPhy, wifiMac, m_nodes);
}

void
RtMhrExample::InstallInternetStack()
{
    NS_LOG_INFO("Installing internet stack with RT-MHR routing");

    InternetStackHelper internet;

    // Install RT-MHR routing
    RtMhrHelper rtmhr;

    // Configure RT-MHR parameters
    rtmhr.Set("HelloInterval", TimeValue(Seconds(1.0)));
    rtmhr.Set("NeighborTimeout", TimeValue(Seconds(3.0)));
    rtmhr.Set("RouteTimeout", TimeValue(Seconds(30.0)));
    rtmhr.Set("FastLocalRepair", BooleanValue(true));

    internet.SetRoutingHelper(rtmhr);
    internet.Install(m_nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    m_interfaces = ipv4.Assign(m_devices);
}

void
RtMhrExample::InstallApplications()
{
    NS_LOG_INFO("Installing applications");

    // Create UDP echo server on node 0
    UdpEchoServerHelper echoServer(9);
    m_serverApps = echoServer.Install(m_nodes.Get(0));
    m_serverApps.Start(Seconds(1.0));
    m_serverApps.Stop(Seconds(m_totalTime));

    // Create UDP echo clients on other nodes
    UdpEchoClientHelper echoClient(m_interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // Install clients on nodes 1-4
    for (uint32_t i = 1; i < std::min(m_nNodes, 5u); ++i)
    {
        ApplicationContainer clientApp = echoClient.Install(m_nodes.Get(i));
        clientApp.Start(Seconds(10.0 + i * 2.0));
        clientApp.Stop(Seconds(m_totalTime - 10.0));
        m_clientApps.Add(clientApp);
    }

    // Create OnOff applications for background traffic
    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(m_interfaces.GetAddress(m_nNodes - 1), 8080));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("DataRate", DataRateValue(DataRate("512kbps")));
    onoff.SetAttribute("PacketSize", UintegerValue(512));

    // Install background traffic on middle nodes
    if (m_nNodes > 6)
    {
        ApplicationContainer onoffApps = onoff.Install(m_nodes.Get(m_nNodes / 2));
        onoffApps.Start(Seconds(20.0));
        onoffApps.Stop(Seconds(m_totalTime - 20.0));
    }
}

void
RtMhrExample::SetupMobility()
{
    NS_LOG_INFO("Setting up mobility");

    MobilityHelper mobility;

    // Set initial positions
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(m_nodeDistance),
                                  "DeltaY",
                                  DoubleValue(m_nodeDistance),
                                  "GridWidth",
                                  UintegerValue(5),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    // Set mobility model
    mobility.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed",
        StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(m_nodeSpeed) + "]"),
        "Pause",
        StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
        "PositionAllocator",
        PointerValue(CreateObject<RandomBoxPositionAllocator>()));

    mobility.Install(m_nodes);
}

void
RtMhrExample::SetupTracing()
{
    NS_LOG_INFO("Setting up tracing");

    // Enable flow monitor
    FlowMonitorHelper flowmon;
    m_flowMonitor = flowmon.InstallAll();

    // Enable packet capture (commented out to avoid undefined reference)
    // wifiPhy.EnablePcapAll("rtmhr-example", true);

    // Enable ASCII tracing (commented out to avoid undefined reference)
    // AsciiTraceHelper ascii;
    // wifiPhy.EnableAsciiAll(ascii.CreateFileStream("rtmhr-example.tr"));

    // Enable routing table dumps
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("rtmhr-example.routes",
    // std::ios::out); Simulator::Schedule(Seconds(30), &Ipv4RoutingHelper::PrintRoutingTableAllAt,
    //                    &rtmhr, Seconds(30), routingStream);
}

void
RtMhrExample::Run()
{
    NS_LOG_INFO("Starting RT-MHR example simulation");

    CreateNodes();
    CreateDevices();
    InstallInternetStack();
    SetupMobility();
    InstallApplications();
    SetupTracing();

    NS_LOG_INFO("Running simulation for " << m_totalTime << " seconds");

    Simulator::Stop(Seconds(m_totalTime));
    Simulator::Run();

    // Print flow monitor statistics
    m_flowMonitor->CheckForLostPackets();
    FlowMonitorHelper flowHelper;
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = m_flowMonitor->GetFlowStats();

    std::cout << "\n=== Flow Monitor Statistics ===" << std::endl;
    for (auto iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        std::cout << "Flow " << iter->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")" << std::endl;
        std::cout << "  Tx Packets: " << iter->second.txPackets << std::endl;
        std::cout << "  Rx Packets: " << iter->second.rxPackets << std::endl;
        std::cout << "  Throughput: "
                  << iter->second.rxBytes * 8.0 /
                         (iter->second.timeLastRxPacket.GetSeconds() -
                          iter->second.timeFirstTxPacket.GetSeconds()) /
                         1024 / 1024
                  << " Mbps" << std::endl;
        std::cout << "  Mean Delay: " << iter->second.delaySum.GetSeconds() / iter->second.rxPackets
                  << " s" << std::endl;
        std::cout << "  Packet Loss Ratio: "
                  << (iter->second.txPackets - iter->second.rxPackets) * 100.0 /
                         iter->second.txPackets
                  << "%" << std::endl
                  << std::endl;
    }

    Simulator::Destroy();

    NS_LOG_INFO("RT-MHR example simulation completed");
}

int
main(int argc, char* argv[])
{
    bool verbose = false;
    uint32_t nNodes = 10;
    double totalTime = 200.0;
    double nodeSpeed = 10.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("totalTime", "Total simulation time", totalTime);
    cmd.AddValue("nodeSpeed", "Maximum node speed", nodeSpeed);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("RtMhrExample", LOG_LEVEL_INFO);
        LogComponentEnable("RtMhr", LOG_LEVEL_DEBUG);
        LogComponentEnable("RtMhrHelper", LOG_LEVEL_DEBUG);
    }

    // Set global random seed
    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(1);

    RtMhrExample example;
    example.Run();

    return 0;
}

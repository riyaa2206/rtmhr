/*
 * Quick RT-MHR Performance Evaluation
 *
 * This simulation compares RT-MHR against AODV in a simple scenario
 */

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/rtmhr-helper.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QuickEvaluation");

class QuickEvaluation
{
  public:
    QuickEvaluation();
    void Run();

  private:
    void RunScenario(std::string protocol, uint32_t numNodes, double speed);
    void PrintResults(std::string protocol,
                      Ptr<FlowMonitor> monitor,
                      FlowMonitorHelper& flowHelper);

    NodeContainer nodes;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer interfaces;
    uint32_t m_numNodes;
    double m_simulationTime;
    double m_nodeSpeed;
};

QuickEvaluation::QuickEvaluation()
    : m_numNodes(20),
      m_simulationTime(100.0),
      m_nodeSpeed(10.0)
{
}

void
QuickEvaluation::Run()
{
    std::cout << "=== RT-MHR Quick Performance Evaluation ===" << std::endl;
    std::cout << "Nodes: " << m_numNodes << ", Speed: " << m_nodeSpeed << " m/s" << std::endl;
    std::cout << "Simulation Time: " << m_simulationTime << " seconds" << std::endl << std::endl;

    // Test RT-MHR
    std::cout << "Testing RT-MHR Protocol..." << std::endl;
    RunScenario("rtmhr", m_numNodes, m_nodeSpeed);

    std::cout << "\n" << std::string(50, '=') << std::endl;

    // Test AODV for comparison
    std::cout << "Testing AODV Protocol..." << std::endl;
    RunScenario("aodv", m_numNodes, m_nodeSpeed);
}

void
QuickEvaluation::RunScenario(std::string protocol, uint32_t numNodes, double speed)
{
    // Reset configuration
    Config::Reset();
    GlobalValue::Bind("RngSeed", UintegerValue(42));
    GlobalValue::Bind("RngRun", UintegerValue(1));

    // Create nodes
    nodes.Create(numNodes);

    // Setup WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate2Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange",
                                   DoubleValue(300.0));

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("RxSensitivity", DoubleValue(-96.0));

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // Setup mobility
    MobilityHelper mobility;
    Ptr<RandomRectanglePositionAllocator> positionAlloc =
        CreateObject<RandomRectanglePositionAllocator>();
    positionAlloc->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    positionAlloc->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    mobility.SetPositionAllocator(positionAlloc);

    if (speed > 0)
    {
        mobility.SetMobilityModel(
            "ns3::RandomWaypointMobilityModel",
            "Speed",
            StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(speed) + "]"),
            "Pause",
            StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
            "PositionAllocator",
            PointerValue(positionAlloc));
    }
    else
    {
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }

    mobility.Install(nodes);

    // Setup Internet stack with routing protocol
    InternetStackHelper internet;

    if (protocol == "rtmhr")
    {
        RtMhrHelper rtmhr;
        internet.SetRoutingHelper(rtmhr);
    }
    else if (protocol == "aodv")
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
    }

    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    interfaces = address.Assign(devices);

    // Setup applications
    uint16_t port = 9;

    // Create multiple CBR flows
    for (uint32_t i = 0; i < numNodes / 4; ++i)
    {
        uint32_t sourceNode = i;
        uint32_t sinkNode = numNodes - 1 - i;

        // UDP Echo Server
        UdpEchoServerHelper echoServer(port + i);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(sinkNode));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(m_simulationTime - 1.0));

        // UDP Echo Client
        UdpEchoClientHelper echoClient(interfaces.GetAddress(sinkNode), port + i);
        echoClient.SetAttribute("MaxPackets", UintegerValue(1000));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.1)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = echoClient.Install(nodes.Get(sourceNode));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(Seconds(m_simulationTime - 1.0));
    }

    // Setup flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    std::cout << "Running " << protocol << " simulation..." << std::endl;
    Simulator::Stop(Seconds(m_simulationTime));
    Simulator::Run();

    // Print results
    PrintResults(protocol, monitor, flowHelper);

    Simulator::Destroy();
}

void
QuickEvaluation::PrintResults(std::string protocol,
                              Ptr<FlowMonitor> monitor,
                              FlowMonitorHelper& flowHelper)
{
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    double totalJitter = 0.0;
    uint32_t totalTxPackets = 0;
    uint32_t totalRxPackets = 0;
    uint32_t flowCount = 0;

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        FlowMonitor::FlowStats flowStats = i->second;

        if (flowStats.txPackets > 0)
        {
            flowCount++;
            totalTxPackets += flowStats.txPackets;
            totalRxPackets += flowStats.rxPackets;

            double throughput = flowStats.rxBytes * 8.0 / (m_simulationTime - 2.0) / 1024.0; // kbps
            totalThroughput += throughput;

            if (flowStats.rxPackets > 0)
            {
                double avgDelay =
                    flowStats.delaySum.GetSeconds() / flowStats.rxPackets * 1000.0; // ms
                totalDelay += avgDelay;

                if (flowStats.rxPackets > 1)
                {
                    double avgJitter =
                        flowStats.jitterSum.GetSeconds() / (flowStats.rxPackets - 1) * 1000.0; // ms
                    totalJitter += avgJitter;
                }
            }
        }
    }

    // Calculate averages
    double avgThroughput = flowCount > 0 ? totalThroughput / flowCount : 0.0;
    double avgDelay = flowCount > 0 ? totalDelay / flowCount : 0.0;
    double avgJitter = flowCount > 0 ? totalJitter / flowCount : 0.0;
    double packetDeliveryRatio =
        totalTxPackets > 0 ? (double)totalRxPackets / totalTxPackets * 100.0 : 0.0;

    std::cout << "\n=== " << protocol << " Results ===" << std::endl;
    std::cout << "Flows: " << flowCount << std::endl;
    std::cout << "Packet Delivery Ratio: " << packetDeliveryRatio << " %" << std::endl;
    std::cout << "Average Throughput: " << avgThroughput << " kbps" << std::endl;
    std::cout << "Average End-to-End Delay: " << avgDelay << " ms" << std::endl;
    std::cout << "Average Jitter: " << avgJitter << " ms" << std::endl;
    std::cout << "Total Tx Packets: " << totalTxPackets << std::endl;
    std::cout << "Total Rx Packets: " << totalRxPackets << std::endl;
}

int
main(int argc, char* argv[])
{
    // Parse command line arguments
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    QuickEvaluation evaluation;
    evaluation.Run();

    return 0;
}
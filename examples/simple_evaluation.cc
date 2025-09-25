/*
 * Simple RT-MHR Performance Test
 *
 * This simulation tests RT-MHR protocol in a simple scenario
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/rtmhr-helper.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleEvaluation");

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("SimpleEvaluation", LOG_LEVEL_INFO);

    // Simulation parameters
    uint32_t numNodes = 10;
    double simulationTime = 60.0;
    uint32_t packetSize = 1024;

    // Parse command line
    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes", "Number of nodes", numNodes);
    cmd.AddValue("time", "Simulation time", simulationTime);
    cmd.Parse(argc, argv);

    std::cout << "=== RT-MHR Simple Performance Test ===" << std::endl;
    std::cout << "Nodes: " << numNodes << std::endl;
    std::cout << "Simulation Time: " << simulationTime << " seconds" << std::endl;

    // Create nodes
    NodeContainer nodes;
    nodes.Create(numNodes);

    // Setup WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate2Mbps"));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange",
                                   DoubleValue(250.0));

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // Setup mobility - simple grid with closer spacing for full connectivity
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(50.0), // Reduced from 100m to 50m
                                  "DeltaY",
                                  DoubleValue(50.0), // Reduced from 100m to 50m
                                  "GridWidth",
                                  UintegerValue(5),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Setup Internet stack with RT-MHR
    InternetStackHelper internet;
    RtMhrHelper rtmhr;
    internet.SetRoutingHelper(rtmhr);
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Setup applications - UDP Echo
    uint16_t port = 9;

    // Server on last node
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(numNodes - 1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simulationTime - 1.0));

    // Client on first node
    UdpEchoClientHelper echoClient(interfaces.GetAddress(numNodes - 1), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simulationTime - 1.0));

    // Setup flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    std::cout << "Starting simulation..." << std::endl;

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Analyze results
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total Flows: " << stats.size() << std::endl;

    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint32_t totalTxPackets = 0;
    uint32_t totalRxPackets = 0;
    uint32_t flowCount = 0;

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        FlowMonitor::FlowStats flow = i->second;
        if (flow.txPackets > 0)
        {
            flowCount++;
            totalTxPackets += flow.txPackets;
            totalRxPackets += flow.rxPackets;

            double throughput = flow.rxBytes * 8.0 / (simulationTime - 2.0) / 1000.0; // kbps
            totalThroughput += throughput;

            if (flow.rxPackets > 0)
            {
                double avgDelay = flow.delaySum.GetSeconds() / flow.rxPackets * 1000.0; // ms
                totalDelay += avgDelay;
            }

            std::cout << "Flow " << flowCount << ":" << std::endl;
            std::cout << "  Tx Packets: " << flow.txPackets << std::endl;
            std::cout << "  Rx Packets: " << flow.rxPackets << std::endl;
            std::cout << "  Throughput: " << throughput << " kbps" << std::endl;
            if (flow.rxPackets > 0)
            {
                std::cout << "  Avg Delay: " << flow.delaySum.GetSeconds() / flow.rxPackets * 1000.0
                          << " ms" << std::endl;
            }
        }
    }

    // Summary
    double packetDeliveryRatio =
        totalTxPackets > 0 ? (double)totalRxPackets / totalTxPackets * 100.0 : 0.0;
    double avgThroughput = flowCount > 0 ? totalThroughput / flowCount : 0.0;
    double avgDelay = flowCount > 0 ? totalDelay / flowCount : 0.0;

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Packet Delivery Ratio: " << packetDeliveryRatio << " %" << std::endl;
    std::cout << "Average Throughput: " << avgThroughput << " kbps" << std::endl;
    std::cout << "Average Delay: " << avgDelay << " ms" << std::endl;
    std::cout << "Total Tx Packets: " << totalTxPackets << std::endl;
    std::cout << "Total Rx Packets: " << totalRxPackets << std::endl;

    Simulator::Destroy();

    return 0;
}
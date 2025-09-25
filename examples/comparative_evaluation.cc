/*
 * Comparative RT-MHR vs AODV Performance Test
 *
 * This simulation compares RT-MHR and AODV protocols in the same scenarios
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

#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ComparativeEvaluation");

struct ProtocolResults
{
    double pdr;
    double throughput;
    double delay;
    uint32_t txPackets;
    uint32_t rxPackets;
    std::string protocolName;
};

ProtocolResults
runSimulation(std::string protocol, uint32_t numNodes, double simulationTime, uint32_t packetSize)
{
    ProtocolResults results;
    results.protocolName = protocol;

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

    // Setup mobility - grid with closer spacing for connectivity
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(50.0),
                                  "DeltaY",
                                  DoubleValue(50.0),
                                  "GridWidth",
                                  UintegerValue(5),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Setup Internet stack with specified routing protocol
    InternetStackHelper internet;

    if (protocol == "RTMHR")
    {
        RtMhrHelper rtmhr;
        internet.SetRoutingHelper(rtmhr);
    }
    else if (protocol == "AODV")
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
    }

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
    echoClient.SetAttribute("MaxPackets", UintegerValue(1000));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simulationTime - 1.0));

    // Setup flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Collect statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint32_t totalTxPackets = 0;
    uint32_t totalRxPackets = 0;
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint32_t flowCount = 0;

    for (auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        totalTxPackets += flow.second.txPackets;
        totalRxPackets += flow.second.rxPackets;

        if (flow.second.rxPackets > 0)
        {
            double throughput = flow.second.rxBytes * 8.0 / (simulationTime - 2.0) / 1000.0; // kbps
            totalThroughput += throughput;

            double avgDelay = flow.second.delaySum.GetMilliSeconds() / flow.second.rxPackets;
            totalDelay += avgDelay;
            flowCount++;
        }
    }

    results.txPackets = totalTxPackets;
    results.rxPackets = totalRxPackets;
    results.pdr = totalTxPackets > 0 ? (double)totalRxPackets / totalTxPackets * 100.0 : 0.0;
    results.throughput = flowCount > 0 ? totalThroughput / flowCount : 0.0;
    results.delay = flowCount > 0 ? totalDelay / flowCount : 0.0;

    Simulator::Destroy();
    return results;
}

int
main(int argc, char* argv[])
{
    // Simulation parameters
    uint32_t numNodes = 10;
    double simulationTime = 30.0;
    uint32_t packetSize = 1024;
    bool testRtmhr = true;
    bool testAodv = true;

    // Parse command line
    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes", "Number of nodes", numNodes);
    cmd.AddValue("time", "Simulation time", simulationTime);
    cmd.AddValue("size", "Packet size", packetSize);
    cmd.AddValue("rtmhr", "Test RT-MHR protocol", testRtmhr);
    cmd.AddValue("aodv", "Test AODV protocol", testAodv);
    cmd.Parse(argc, argv);

    std::cout << "=== RT-MHR vs AODV Comparative Evaluation ===" << std::endl;
    std::cout << "Nodes: " << numNodes << std::endl;
    std::cout << "Simulation Time: " << simulationTime << " seconds" << std::endl;
    std::cout << "Packet Size: " << packetSize << " bytes" << std::endl;
    std::cout << std::endl;

    std::vector<ProtocolResults> results;

    if (testRtmhr)
    {
        std::cout << "Testing RT-MHR protocol..." << std::endl;
        ProtocolResults rtmhrResults = runSimulation("RTMHR", numNodes, simulationTime, packetSize);
        results.push_back(rtmhrResults);
        std::cout << "RT-MHR completed." << std::endl;
    }

    if (testAodv)
    {
        std::cout << "Testing AODV protocol..." << std::endl;
        ProtocolResults aodvResults = runSimulation("AODV", numNodes, simulationTime, packetSize);
        results.push_back(aodvResults);
        std::cout << "AODV completed." << std::endl;
    }

    // Display results
    std::cout << std::endl << "=== Comparative Results ===" << std::endl;
    std::cout << std::left;
    std::cout << std::setw(10) << "Protocol" << std::setw(12) << "PDR (%)" << std::setw(15)
              << "Throughput" << std::setw(12) << "Delay (ms)" << std::setw(10) << "Tx Pkts"
              << std::setw(10) << "Rx Pkts" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    for (const auto& result : results)
    {
        std::cout << std::setw(10) << result.protocolName << std::setw(12) << std::fixed
                  << std::setprecision(2) << result.pdr << std::setw(15) << std::fixed
                  << std::setprecision(2) << result.throughput << " kbps" << std::setw(12)
                  << std::fixed << std::setprecision(2) << result.delay << std::setw(10)
                  << result.txPackets << std::setw(10) << result.rxPackets << std::endl;
    }

    // Performance comparison
    if (results.size() == 2)
    {
        std::cout << std::endl << "=== Performance Comparison ===" << std::endl;
        ProtocolResults rtmhr = results[0].protocolName == "RTMHR" ? results[0] : results[1];
        ProtocolResults aodv = results[0].protocolName == "AODV" ? results[0] : results[1];

        double pdrDiff = rtmhr.pdr - aodv.pdr;
        double throughputDiff = rtmhr.throughput - aodv.throughput;
        double delayDiff = rtmhr.delay - aodv.delay;

        std::cout << "RT-MHR vs AODV:" << std::endl;
        std::cout << "  PDR difference: " << std::showpos << std::fixed << std::setprecision(2)
                  << pdrDiff << "%" << std::endl;
        std::cout << "  Throughput difference: " << std::showpos << std::fixed
                  << std::setprecision(2) << throughputDiff << " kbps" << std::endl;
        std::cout << "  Delay difference: " << std::showpos << std::fixed << std::setprecision(2)
                  << delayDiff << " ms" << std::endl;

        std::cout << std::noshowpos << std::endl;
        if (pdrDiff > 0)
            std::cout << "✓ RT-MHR has better packet delivery" << std::endl;
        else if (pdrDiff < 0)
            std::cout << "✓ AODV has better packet delivery" << std::endl;
        else
            std::cout << "= Equal packet delivery performance" << std::endl;

        if (throughputDiff > 0)
            std::cout << "✓ RT-MHR has higher throughput" << std::endl;
        else if (throughputDiff < 0)
            std::cout << "✓ AODV has higher throughput" << std::endl;
        else
            std::cout << "= Equal throughput performance" << std::endl;

        if (delayDiff < 0)
            std::cout << "✓ RT-MHR has lower delay" << std::endl;
        else if (delayDiff > 0)
            std::cout << "✓ AODV has lower delay" << std::endl;
        else
            std::cout << "= Equal delay performance" << std::endl;
    }

    return 0;
}
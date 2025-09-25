#!/usr/bin/env python3
"""
RT-MHR Protocol Evaluation Simulation Script

This script runs comprehensive simulations to evaluate the RT-MHR protocol
against baseline protocols (AODV, OLSR, DSR) under various scenarios.
"""

import os
import sys
import subprocess
import json
import csv
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from concurrent.futures import ThreadPoolExecutor
import argparse
from pathlib import Path

class RTMHRSimulation:
    """Main simulation class for RT-MHR evaluation"""
    
    def __init__(self, ns3_path="/home/ramas/ns-allinone-3.43/ns-3.43"):
        self.ns3_path = ns3_path
        self.results_dir = Path("rtmhr_results")
        self.results_dir.mkdir(exist_ok=True)
        
        # Simulation parameters
        self.protocols = ["rtmhr", "aodv", "olsr", "dsr"]
        self.node_counts = [30, 50, 70, 100]
        self.speeds = [0, 5, 10, 15, 20]  # m/s
        self.traffic_loads = ["low", "medium", "high"]
        self.simulation_time = 200  # seconds
        self.seeds = [1, 2, 3, 4, 5]  # Multiple runs for statistical significance
        
    def create_simulation_script(self, protocol, nodes, speed, traffic, seed):
        """Create NS-3 simulation script for given parameters"""
        script_content = f'''
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsr-module.h"
#include "ns3/rtmhr-helper.h"

using namespace ns3;

int main(int argc, char* argv[])
{{
    uint32_t nNodes = {nodes};
    double maxSpeed = {speed};
    std::string protocol = "{protocol}";
    std::string trafficLoad = "{traffic}";
    uint32_t seed = {seed};
    double simTime = {self.simulation_time};
    
    // Set random seed
    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(seed);
    
    // Create nodes
    NodeContainer nodes;
    nodes.Create(nNodes);
    
    // WiFi configuration
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("DsssRate1Mbps"),
                                "ControlMode", StringValue("DsssRate1Mbps"));
    
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
    
    // Mobility model
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
                                 "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"),
                                 "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"),
                                 "Z", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                             "Speed", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(maxSpeed) + "]"),
                             "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                             "PositionAllocator", PointerValue(CreateObject<RandomBoxPositionAllocator>()));
    
    mobility.Install(nodes);
    
    // Internet stack with routing protocol
    InternetStackHelper internet;
    
    if (protocol == "rtmhr") {{
        RtMhrHelper rtmhr;
        internet.SetRoutingHelper(rtmhr);
    }} else if (protocol == "aodv") {{
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
    }} else if (protocol == "olsr") {{
        OlsrHelper olsr;
        internet.SetRoutingHelper(olsr);
    }} else if (protocol == "dsr") {{
        DsrHelper dsr;
        DsrMainHelper dsrMain;
        dsrMain.Install(dsr, nodes);
        internet.Install(nodes);
        goto skip_normal_install;
    }}
    
    internet.Install(nodes);
    
    skip_normal_install:
    
    // IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
    
    // Applications based on traffic load
    uint32_t nFlows = (trafficLoad == "low") ? 5 : (trafficLoad == "medium") ? 10 : 20;
    
    // Install UDP echo servers
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps;
    for (uint32_t i = 0; i < nFlows; ++i) {{
        serverApps.Add(echoServer.Install(nodes.Get(i % nNodes)));
    }}
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    
    // Install UDP echo clients
    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < nFlows; ++i) {{
        uint32_t server = i % nNodes;
        uint32_t client = (i + nNodes/2) % nNodes;
        
        UdpEchoClientHelper echoClient(interfaces.GetAddress(server), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(1000));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));
        
        ApplicationContainer clientApp = echoClient.Install(nodes.Get(client));
        clientApp.Start(Seconds(10.0 + i * 2.0));
        clientApp.Stop(Seconds(simTime - 10.0));
        clientApps.Add(clientApp);
    }}
    
    // Flow monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    
    // Collect results
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    
    // Write results to CSV
    std::ofstream csvFile("results_" + protocol + "_" + std::to_string(nNodes) + "_" + 
                         std::to_string((int)maxSpeed) + "_" + trafficLoad + "_" + 
                         std::to_string(seed) + ".csv");
    
    csvFile << "FlowId,SourceIP,DestIP,TxPackets,RxPackets,TxBytes,RxBytes,DelaySum,Throughput,PDR,AvgDelay\\n";
    
    for (auto& flow : stats) {{
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        
        double throughput = flow.second.rxBytes * 8.0 / 
                          (flow.second.timeLastRxPacket.GetSeconds() - 
                           flow.second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024;
        
        double pdr = (flow.second.txPackets > 0) ? 
                    (double)flow.second.rxPackets / (double)flow.second.txPackets : 0;
        
        double avgDelay = (flow.second.rxPackets > 0) ? 
                         flow.second.delaySum.GetSeconds() / flow.second.rxPackets : 0;
        
        csvFile << flow.first << "," 
                << t.sourceAddress << "," 
                << t.destinationAddress << ","
                << flow.second.txPackets << ","
                << flow.second.rxPackets << ","
                << flow.second.txBytes << ","
                << flow.second.rxBytes << ","
                << flow.second.delaySum.GetSeconds() << ","
                << throughput << ","
                << pdr << ","
                << avgDelay << "\\n";
    }}
    
    csvFile.close();
    
    Simulator::Destroy();
    return 0;
}}
'''
        return script_content
    
    def run_single_simulation(self, params):
        """Run a single simulation with given parameters"""
        protocol, nodes, speed, traffic, seed = params
        
        # Create simulation script
        script_content = self.create_simulation_script(protocol, nodes, speed, traffic, seed)
        script_file = self.results_dir / f"sim_{protocol}_{nodes}_{speed}_{traffic}_{seed}.cc"
        
        with open(script_file, 'w') as f:
            f.write(script_content)
        
        # Copy to NS-3 scratch directory
        scratch_file = Path(self.ns3_path) / "scratch" / f"rtmhr_eval_{protocol}_{nodes}_{speed}_{traffic}_{seed}.cc"
        subprocess.run(["cp", str(script_file), str(scratch_file)])
        
        # Run simulation
        cmd = [
            "./ns3", "run", 
            f"rtmhr_eval_{protocol}_{nodes}_{speed}_{traffic}_{seed}",
            "--cwd", str(self.results_dir)
        ]
        
        try:
            result = subprocess.run(cmd, cwd=self.ns3_path, 
                                  capture_output=True, text=True, timeout=300)
            if result.returncode != 0:
                print(f"Simulation failed: {params}")
                print(f"Error: {result.stderr}")
                return None
            
            return f"results_{protocol}_{nodes}_{speed}_{traffic}_{seed}.csv"
        
        except subprocess.TimeoutExpired:
            print(f"Simulation timed out: {params}")
            return None
        except Exception as e:
            print(f"Simulation error: {params}, {e}")
            return None
    
    def run_all_simulations(self, max_workers=4):
        """Run all simulation combinations"""
        print("Starting RT-MHR evaluation simulations...")
        
        # Generate all parameter combinations
        params_list = []
        for protocol in self.protocols:
            for nodes in self.node_counts:
                for speed in self.speeds:
                    for traffic in self.traffic_loads:
                        for seed in self.seeds:
                            params_list.append((protocol, nodes, speed, traffic, seed))
        
        print(f"Total simulations to run: {len(params_list)}")
        
        # Run simulations in parallel
        completed_results = []
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            results = list(executor.map(self.run_single_simulation, params_list))
        
        # Filter successful results
        completed_results = [r for r in results if r is not None]
        print(f"Completed simulations: {len(completed_results)}")
        
        return completed_results
    
    def analyze_results(self, result_files):
        """Analyze simulation results and generate statistics"""
        print("Analyzing simulation results...")
        
        all_data = []
        
        for result_file in result_files:
            if not (self.results_dir / result_file).exists():
                continue
            
            # Parse filename for parameters
            parts = result_file.replace('results_', '').replace('.csv', '').split('_')
            if len(parts) < 5:
                continue
            
            protocol, nodes, speed, traffic, seed = parts
            
            try:
                df = pd.read_csv(self.results_dir / result_file)
                
                if df.empty:
                    continue
                
                # Calculate aggregate metrics
                metrics = {
                    'protocol': protocol,
                    'nodes': int(nodes),
                    'speed': int(speed),
                    'traffic': traffic,
                    'seed': int(seed),
                    'avg_throughput': df['Throughput'].mean(),
                    'avg_delay': df['AvgDelay'].mean(),
                    'avg_pdr': df['PDR'].mean(),
                    'total_flows': len(df),
                    'successful_flows': (df['PDR'] > 0).sum()
                }
                
                all_data.append(metrics)
                
            except Exception as e:
                print(f"Error processing {result_file}: {e}")
                continue
        
        if not all_data:
            print("No valid data found!")
            return
        
        # Create comprehensive results DataFrame
        results_df = pd.DataFrame(all_data)
        
        # Save raw results
        results_df.to_csv(self.results_dir / "aggregate_results.csv", index=False)
        
        # Generate summary statistics
        self.generate_summary_stats(results_df)
        
        # Generate visualizations
        self.generate_visualizations(results_df)
        
        return results_df
    
    def generate_summary_stats(self, df):
        """Generate summary statistics"""
        print("Generating summary statistics...")
        
        summary_stats = {}
        
        # Group by protocol and calculate means
        protocol_stats = df.groupby('protocol').agg({
            'avg_throughput': ['mean', 'std'],
            'avg_delay': ['mean', 'std'],
            'avg_pdr': ['mean', 'std']
        }).round(4)
        
        summary_stats['protocol_comparison'] = protocol_stats
        
        # Performance vs node count
        node_stats = df.groupby(['protocol', 'nodes']).agg({
            'avg_throughput': 'mean',
            'avg_delay': 'mean',
            'avg_pdr': 'mean'
        }).round(4)
        
        summary_stats['nodes_comparison'] = node_stats
        
        # Performance vs mobility
        speed_stats = df.groupby(['protocol', 'speed']).agg({
            'avg_throughput': 'mean',
            'avg_delay': 'mean',
            'avg_pdr': 'mean'
        }).round(4)
        
        summary_stats['speed_comparison'] = speed_stats
        
        # Save summary statistics
        with open(self.results_dir / "summary_statistics.json", 'w') as f:
            json.dump(summary_stats, f, indent=2, default=str)
        
        print("Summary statistics saved to summary_statistics.json")
    
    def generate_visualizations(self, df):
        """Generate performance visualization plots"""
        print("Generating visualization plots...")
        
        plt.style.use('default')
        
        # 1. Protocol Comparison - Average Performance
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        protocol_means = df.groupby('protocol').agg({
            'avg_throughput': 'mean',
            'avg_delay': 'mean',
            'avg_pdr': 'mean'
        })
        
        # Throughput comparison
        axes[0].bar(protocol_means.index, protocol_means['avg_throughput'])
        axes[0].set_title('Average Throughput by Protocol')
        axes[0].set_ylabel('Throughput (Mbps)')
        axes[0].set_xlabel('Protocol')
        
        # Delay comparison
        axes[1].bar(protocol_means.index, protocol_means['avg_delay'])
        axes[1].set_title('Average Delay by Protocol')
        axes[1].set_ylabel('Delay (seconds)')
        axes[1].set_xlabel('Protocol')
        
        # PDR comparison
        axes[2].bar(protocol_means.index, protocol_means['avg_pdr'])
        axes[2].set_title('Average PDR by Protocol')
        axes[2].set_ylabel('Packet Delivery Ratio')
        axes[2].set_xlabel('Protocol')
        
        plt.tight_layout()
        plt.savefig(self.results_dir / "protocol_comparison.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        # 2. Performance vs Node Count
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        for protocol in self.protocols:
            protocol_data = df[df['protocol'] == protocol]
            node_stats = protocol_data.groupby('nodes').agg({
                'avg_throughput': 'mean',
                'avg_delay': 'mean',
                'avg_pdr': 'mean'
            })
            
            axes[0].plot(node_stats.index, node_stats['avg_throughput'], 
                        marker='o', label=protocol)
            axes[1].plot(node_stats.index, node_stats['avg_delay'], 
                        marker='o', label=protocol)
            axes[2].plot(node_stats.index, node_stats['avg_pdr'], 
                        marker='o', label=protocol)
        
        axes[0].set_title('Throughput vs Node Count')
        axes[0].set_xlabel('Number of Nodes')
        axes[0].set_ylabel('Throughput (Mbps)')
        axes[0].legend()
        axes[0].grid(True)
        
        axes[1].set_title('Delay vs Node Count')
        axes[1].set_xlabel('Number of Nodes')
        axes[1].set_ylabel('Delay (seconds)')
        axes[1].legend()
        axes[1].grid(True)
        
        axes[2].set_title('PDR vs Node Count')
        axes[2].set_xlabel('Number of Nodes')
        axes[2].set_ylabel('Packet Delivery Ratio')
        axes[2].legend()
        axes[2].grid(True)
        
        plt.tight_layout()
        plt.savefig(self.results_dir / "performance_vs_nodes.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        # 3. Performance vs Mobility
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        for protocol in self.protocols:
            protocol_data = df[df['protocol'] == protocol]
            speed_stats = protocol_data.groupby('speed').agg({
                'avg_throughput': 'mean',
                'avg_delay': 'mean',
                'avg_pdr': 'mean'
            })
            
            axes[0].plot(speed_stats.index, speed_stats['avg_throughput'], 
                        marker='o', label=protocol)
            axes[1].plot(speed_stats.index, speed_stats['avg_delay'], 
                        marker='o', label=protocol)
            axes[2].plot(speed_stats.index, speed_stats['avg_pdr'], 
                        marker='o', label=protocol)
        
        axes[0].set_title('Throughput vs Node Speed')
        axes[0].set_xlabel('Max Speed (m/s)')
        axes[0].set_ylabel('Throughput (Mbps)')
        axes[0].legend()
        axes[0].grid(True)
        
        axes[1].set_title('Delay vs Node Speed')
        axes[1].set_xlabel('Max Speed (m/s)')
        axes[1].set_ylabel('Delay (seconds)')
        axes[1].legend()
        axes[1].grid(True)
        
        axes[2].set_title('PDR vs Node Speed')
        axes[2].set_xlabel('Max Speed (m/s)')
        axes[2].set_ylabel('Packet Delivery Ratio')
        axes[2].legend()
        axes[2].grid(True)
        
        plt.tight_layout()
        plt.savefig(self.results_dir / "performance_vs_mobility.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print("Visualization plots saved successfully!")


def main():
    parser = argparse.ArgumentParser(description='RT-MHR Protocol Evaluation')
    parser.add_argument('--mode', choices=['simulate', 'analyze', 'both'], 
                       default='both', help='Operation mode')
    parser.add_argument('--ns3-path', default='/home/ramas/ns-allinone-3.43/ns-3.43',
                       help='Path to NS-3 installation')
    parser.add_argument('--workers', type=int, default=4,
                       help='Number of parallel simulation workers')
    
    args = parser.parse_args()
    
    # Create simulation instance
    sim = RTMHRSimulation(args.ns3_path)
    
    if args.mode in ['simulate', 'both']:
        # Run simulations
        result_files = sim.run_all_simulations(max_workers=args.workers)
        
        if args.mode == 'simulate':
            print(f"Simulations completed. Results saved in {sim.results_dir}")
            return
    
    if args.mode in ['analyze', 'both']:
        # Find existing result files
        result_files = [f.name for f in sim.results_dir.glob("results_*.csv")]
        
        if not result_files:
            print("No result files found. Run simulations first.")
            return
        
        # Analyze results
        results_df = sim.analyze_results(result_files)
        
        print(f"Analysis completed. Results saved in {sim.results_dir}")
        print("Key files generated:")
        print("- aggregate_results.csv: Raw aggregated data")
        print("- summary_statistics.json: Summary statistics")
        print("- protocol_comparison.png: Protocol performance comparison")
        print("- performance_vs_nodes.png: Scalability analysis")
        print("- performance_vs_mobility.png: Mobility impact analysis")


if __name__ == "__main__":
    main()
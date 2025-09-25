#!/usr/bin/env python3
"""
RT-MHR Protocol Scaling Test
Tests performance across different network sizes
"""

import subprocess
import sys
import time

def run_simulation(nodes, duration=10):
    """Run a simulation with specified parameters"""
    cmd = [
        "./ns3", "run", "simple-evaluation", "--",
        f"--time={duration}",
        f"--nodes={nodes}"
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode != 0:
            print(f"Error running simulation with {nodes} nodes:")
            print(result.stderr)
            return None
        
        # Parse output for key metrics
        output = result.stdout
        lines = output.split('\n')
        
        metrics = {}
        for line in lines:
            if "Packet Delivery Ratio:" in line:
                metrics['pdr'] = float(line.split(':')[1].strip().replace('%', ''))
            elif "Average Throughput:" in line:
                metrics['throughput'] = float(line.split(':')[1].strip().replace('kbps', ''))
            elif "Average Delay:" in line:
                metrics['delay'] = float(line.split(':')[1].strip().replace('ms', ''))
            elif "Total Tx Packets:" in line:
                metrics['tx_packets'] = int(line.split(':')[1].strip())
            elif "Total Rx Packets:" in line:
                metrics['rx_packets'] = int(line.split(':')[1].strip())
        
        return metrics
    
    except subprocess.TimeoutExpired:
        print(f"Simulation with {nodes} nodes timed out")
        return None
    except Exception as e:
        print(f"Error with {nodes} nodes: {e}")
        return None

def main():
    print("=== RT-MHR Protocol Scaling Test ===")
    print("Testing performance across different network sizes")
    print("Grid spacing: 50m, WiFi range: 250m")
    print()
    
    # Test different network sizes
    test_sizes = [3, 4, 5, 6, 8, 10, 12, 15, 18, 20, 25]
    
    results = []
    
    for nodes in test_sizes:
        print(f"Testing {nodes} nodes...", end=' ', flush=True)
        metrics = run_simulation(nodes)
        
        if metrics:
            results.append((nodes, metrics))
            pdr = metrics.get('pdr', 0)
            throughput = metrics.get('throughput', 0)
            delay = metrics.get('delay', 0)
            print(f"PDR: {pdr:.1f}%, Throughput: {throughput:.2f} kbps, Delay: {delay:.2f} ms")
        else:
            print("FAILED")
        
        time.sleep(0.5)  # Brief pause between tests
    
    print("\n=== Scaling Test Results Summary ===")
    print(f"{'Nodes':<6} {'PDR (%)':<8} {'Throughput (kbps)':<18} {'Delay (ms)':<12} {'Status'}")
    print("-" * 55)
    
    for nodes, metrics in results:
        pdr = metrics.get('pdr', 0)
        throughput = metrics.get('throughput', 0)
        delay = metrics.get('delay', 0)
        status = "✓ PASS" if pdr == 100 else "✗ FAIL"
        
        print(f"{nodes:<6} {pdr:<8.1f} {throughput:<18.2f} {delay:<12.2f} {status}")
    
    # Find the largest successful network
    max_working_nodes = 0
    for nodes, metrics in results:
        if metrics.get('pdr', 0) == 100:
            max_working_nodes = max(max_working_nodes, nodes)
    
    print(f"\nLargest fully-connected network: {max_working_nodes} nodes")
    print(f"Connectivity limit reached at: {max_working_nodes + 1}+ nodes")

if __name__ == "__main__":
    main()
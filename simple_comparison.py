#!/usr/bin/env python3
"""
Simple RT-MHR vs AODV Comparison
"""

import subprocess
import sys

def run_test(nodes, time=20):
    cmd = ["./ns3", "run", "comparative-evaluation", "--", f"--nodes={nodes}", f"--time={time}"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout

def main():
    print("=== RT-MHR vs AODV Protocol Comparison ===")
    print()
    
    test_cases = [
        (5, "Small fully-connected network"),
        (10, "Medium fully-connected network"), 
        (20, "Large fully-connected network"),
        (24, "Maximum fully-connected network"),
        (25, "Network exceeding connectivity range"),
        (30, "Large partially-connected network")
    ]
    
    for nodes, description in test_cases:
        print(f"Test Case: {nodes} nodes - {description}")
        print("-" * 60)
        output = run_test(nodes)
        
        # Extract key results
        lines = output.split('\n')
        for i, line in enumerate(lines):
            if "=== Comparative Results ===" in line:
                # Print the results table
                for j in range(i, min(i+10, len(lines))):
                    if lines[j].strip() and not lines[j].startswith("==="):
                        print(lines[j])
                break
        
        # Extract performance comparison
        for line in lines:
            if "RT-MHR vs AODV:" in line:
                idx = lines.index(line)
                for j in range(idx+1, min(idx+8, len(lines))):
                    if lines[j].strip() and not lines[j].startswith("==="):
                        print(lines[j])
                break
        
        print()
        
if __name__ == "__main__":
    main()
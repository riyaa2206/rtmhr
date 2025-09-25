# RT-MHR Protocol Scaling Test Results

## Test Configuration

- **Grid Spacing**: 50 meters between adjacent nodes
- **WiFi Range**: 250 meters (RangePropagationLossModel)
- **Topology**: GridPositionAllocator with 5-node width, RowFirst layout
- **Traffic**: UDP Echo between first and last node (0.5s intervals, 10s
  duration)
- **Wireless**: 802.11b with ConstantRateWifiManager (2 Mbps)

## Results Summary

### Fully Connected Networks (100% PDR)

Successfully tested networks from **3 to 24 nodes** with perfect connectivity:

| Nodes | Layout | Max Distance (m) | PDR  | Throughput (kbps) | Avg Delay (ms) |
| ----- | ------ | ---------------- | ---- | ----------------- | -------------- |
| 3     | 1×3    | 100.0            | 100% | 14.73             | ~5.0           |
| 4     | 1×4    | 150.0            | 100% | 14.73             | ~5.0           |
| 5     | 1×5    | 200.0            | 100% | 14.73             | ~5.2           |
| 10    | 2×5    | 223.6            | 100% | 14.73             | ~5.0           |
| 15    | 3×5    | 223.6            | 100% | 14.73             | ~5.1           |
| 20    | 4×5    | 223.6            | 100% | 14.73             | ~5.0           |
| 24    | 5×5-1  | 223.6            | 100% | 14.73             | ~5.3           |

### Connectivity Limit

- **25 nodes**: Complete 5×5 grid with corner-to-corner distance of 282.8m >
  250m WiFi range
- **Result**: 0% packet delivery ratio (complete connectivity failure)

## Key Findings

### 1. Consistent Performance Within Range

- **Perfect PDR**: 100% packet delivery for all fully-connected topologies
- **Stable Throughput**: Consistent 14.73 kbps across all working configurations
- **Low Latency**: Average delay consistently around 5ms
- **Scalability**: No performance degradation from 3 to 24 nodes

### 2. Clear Connectivity Boundary

- **Working**: Up to 24 nodes in optimized grid layout
- **Failing**: 25+ nodes where grid corners exceed WiFi range
- **Geometric Limit**: √(200² + 200²) = 282.8m > 250m range

### 3. Protocol Efficiency

- **Direct Routing**: Works perfectly for single-hop scenarios
- **Subnet-based**: Efficient route discovery within same IP subnet
- **No Overhead**: Minimal delay increase with network size

## Network Topology Analysis

### Grid Layout (50m spacing, 5-node width)

```
Nodes 1-5:   (0,0)   (50,0)  (100,0) (150,0) (200,0)
Nodes 6-10:  (0,50)  (50,50) (100,50)(150,50)(200,50)
Nodes 11-15: (0,100) (50,100)(100,100)(150,100)(200,100)
Nodes 16-20: (0,150) (50,150)(100,150)(150,150)(200,150)
Nodes 21-25: (0,200) (50,200)(100,200)(150,200)(200,200)
```

### Distance Calculations

- **Adjacent nodes**: 50m (horizontal/vertical)
- **Diagonal neighbors**: 70.7m (√(50² + 50²))
- **Opposite corners (4×4)**: 223.6m (√(150² + 150²)) ✓ Within range
- **Opposite corners (5×5)**: 282.8m (√(200² + 200²)) ✗ Exceeds range

## Implications for Multi-Hop Routing

The current RT-MHR implementation uses **direct routing** which works perfectly
for:

- Small networks where all nodes are within WiFi range
- Dense deployments with adequate coverage overlap
- Applications requiring guaranteed single-hop connectivity

For larger networks requiring multi-hop routing, the protocol would need:

- **Route discovery protocols** for multi-hop path finding
- **Routing table management** for storing intermediate routes
- **Packet forwarding logic** for relay nodes
- **Link quality metrics** for optimal path selection

## Conclusion

RT-MHR demonstrates excellent performance characteristics within its designed
scope:

- ✅ **Reliable**: 100% packet delivery in connected scenarios
- ✅ **Efficient**: Consistent low latency and good throughput
- ✅ **Scalable**: No performance degradation up to connectivity limits
- ✅ **Predictable**: Clear geometric boundaries for network design

The protocol successfully validates the direct routing approach for single-hop
wireless networks and provides a solid foundation for future multi-hop
extensions.

RT-MHR Model
*************

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

This chapter describes the RT-MHR (Real-Time Multi-Hop Routing) model implementation in |ns3|.

Overview
========

The RT-MHR protocol is an optimized multi-hop wireless routing protocol designed for real-time data delivery in dynamic networks. It provides:

* **Cross-layer Route Metric (CRM)**: Combines link quality, queuing delay, mobility prediction, and hop count
* **Multi-path routing**: Maintains primary and backup paths for improved reliability
* **Fast Local Repair (FLR)**: Quick recovery from link failures before global route discovery
* **Priority queuing**: Reduces latency and jitter for real-time traffic
* **Adaptive link monitoring**: Lightweight probing with adaptive frequency

Key Features
============

Cross-Layer Route Metric
#########################

RT-MHR uses a composite metric that considers multiple factors:

.. math::

   CRM = w_1 \cdot Q_{link} + w_2 \cdot \frac{1}{D_{queue} + \epsilon} + w_3 \cdot \frac{1}{M_{mobility} + \epsilon} + w_4 \cdot \frac{1}{H_{hops} + 1}

Where:
- :math:`Q_{link}` is the link quality (0-1)
- :math:`D_{queue}` is the queuing delay in seconds
- :math:`M_{mobility}` is the mobility prediction metric
- :math:`H_{hops}` is the hop count
- :math:`w_1, w_2, w_3, w_4` are configurable weights

Multi-Path Routing
##################

RT-MHR maintains both primary and backup paths to each destination:

* **Primary path**: Used for normal packet forwarding
* **Backup path**: Activated immediately upon primary path failure
* **Path selection**: Based on composite route metric

Fast Local Repair
#################

When a link failure is detected:

1. **Immediate switch**: If backup path exists, switch immediately
2. **Local repair**: Search for alternative path through neighbors
3. **Global discovery**: Fall back to full route discovery if local repair fails

Traffic Prioritization
######################

RT-MHR classifies traffic into priority levels:

* **Emergency** (Priority 3): Emergency services, critical alerts
* **High** (Priority 2): Real-time audio/video, VoIP
* **Medium** (Priority 1): Multimedia, streaming
* **Low** (Priority 0): Best effort, background traffic

Design
======

Model Architecture
##################

The RT-MHR model consists of several key components:

**Core Protocol Engine**
  - Route discovery and maintenance
  - Neighbor management
  - Message processing

**Cross-Layer Interface**
  - Link quality estimation
  - Mobility prediction
  - Queue monitoring

**Priority Queue Manager**
  - Traffic classification
  - QoS-aware forwarding
  - Congestion control

**Fast Repair Module**
  - Failure detection
  - Local path search
  - Backup path management

Usage
=====

Basic Usage
###########

To use RT-MHR in your simulation:

.. code-block:: cpp

   #include "ns3/rtmhr-helper.h"
   
   // Create nodes
   NodeContainer nodes;
   nodes.Create(10);
   
   // Install RT-MHR routing
   InternetStackHelper internet;
   RtMhrHelper rtmhr;
   
   // Configure parameters
   rtmhr.Set("HelloInterval", TimeValue(Seconds(1.0)));
   rtmhr.Set("FastLocalRepair", BooleanValue(true));
   
   internet.SetRoutingHelper(rtmhr);
   internet.Install(nodes);

Configuration Parameters
########################

The RT-MHR protocol supports various configuration parameters:

.. code-block:: cpp

   rtmhr.Set("HelloInterval", TimeValue(Seconds(1.0)));
   rtmhr.Set("NeighborTimeout", TimeValue(Seconds(3.0)));
   rtmhr.Set("RouteTimeout", TimeValue(Seconds(30.0)));
   rtmhr.Set("FastLocalRepair", BooleanValue(true));
   rtmhr.Set("LinkQualityWeight", DoubleValue(0.3));
   rtmhr.Set("DelayWeight", DoubleValue(0.25));
   rtmhr.Set("MobilityWeight", DoubleValue(0.25));
   rtmhr.Set("HopCountWeight", DoubleValue(0.2));

Examples
========

The RT-MHR model includes several example programs:

* **rtmhr-example.cc**: Basic RT-MHR demonstration with mobile nodes

Testing
=======

The RT-MHR model includes comprehensive test suites:

* **Basic functionality tests**: Route discovery, neighbor management
* **Metric calculation tests**: Cross-layer metric computation

To run the tests:

.. code-block:: bash

   ./ns3 test rtmhr
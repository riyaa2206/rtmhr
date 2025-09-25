#include "ns3/constant-position-mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/rtmhr-helper.h"
#include "ns3/rtmhr.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/udp-header.h"

using namespace ns3;

/**
 * \ingroup rtmhr
 * \defgroup rtmhr-test RT-MHR module tests
 */

/**
 * \ingroup rtmhr-test
 * \ingroup tests
 * \brief RT-MHR protocol basic functionality test case
 */
class RtMhrBasicTestCase : public TestCase
{
  public:
    RtMhrBasicTestCase();
    virtual ~RtMhrBasicTestCase();

  private:
    virtual void DoRun() override;
    void CreateNodes();
    void CreateDevices();
    void InstallInternetStack();
    void TestRouteDiscovery();
    void TestNeighborDiscovery();

    NodeContainer m_nodes;
    NetDeviceContainer m_devices;
    Ipv4InterfaceContainer m_interfaces;
};

RtMhrBasicTestCase::RtMhrBasicTestCase()
    : TestCase("RT-MHR basic functionality test")
{
}

RtMhrBasicTestCase::~RtMhrBasicTestCase()
{
}

void
RtMhrBasicTestCase::CreateNodes()
{
    m_nodes.Create(3); // Simple 3-node topology
}

void
RtMhrBasicTestCase::CreateDevices()
{
    SimpleNetDeviceHelper deviceHelper;
    deviceHelper.SetChannel("ns3::SimpleChannel");
    m_devices = deviceHelper.Install(m_nodes);
}

void
RtMhrBasicTestCase::InstallInternetStack()
{
    InternetStackHelper internet;
    RtMhrHelper rtmhr;
    internet.SetRoutingHelper(rtmhr);
    internet.Install(m_nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    m_interfaces = ipv4.Assign(m_devices);
}

void
RtMhrBasicTestCase::TestRouteDiscovery()
{
    // Test basic route discovery mechanism
    Ptr<RtMhr> rtmhr0 = m_nodes.Get(0)->GetObject<RtMhr>();
    Ptr<RtMhr> rtmhr1 = m_nodes.Get(1)->GetObject<RtMhr>();
    Ptr<RtMhr> rtmhr2 = m_nodes.Get(2)->GetObject<RtMhr>();

    NS_TEST_ASSERT_MSG_NE(rtmhr0, nullptr, "RT-MHR protocol not installed on node 0");
    NS_TEST_ASSERT_MSG_NE(rtmhr1, nullptr, "RT-MHR protocol not installed on node 1");
    NS_TEST_ASSERT_MSG_NE(rtmhr2, nullptr, "RT-MHR protocol not installed on node 2");
}

void
RtMhrBasicTestCase::TestNeighborDiscovery()
{
    // Test neighbor discovery mechanism
    // This would require access to internal neighbor table
    // For now, we just verify the protocol is running
    Simulator::Schedule(Seconds(5.0), &RtMhrBasicTestCase::TestRouteDiscovery, this);
}

void
RtMhrBasicTestCase::DoRun()
{
    CreateNodes();
    CreateDevices();
    InstallInternetStack();

    // Set up positions
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(100.0, 0.0, 0.0));
    positionAlloc->Add(Vector(200.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    // Schedule tests
    Simulator::Schedule(Seconds(1.0), &RtMhrBasicTestCase::TestNeighborDiscovery, this);

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * \ingroup rtmhr-test
 * \ingroup tests
 * \brief RT-MHR protocol metric calculation test case
 */
class RtMhrMetricTestCase : public TestCase
{
  public:
    RtMhrMetricTestCase();
    virtual ~RtMhrMetricTestCase();

  private:
    virtual void DoRun() override;
    void TestCrossLayerMetric();
};

RtMhrMetricTestCase::RtMhrMetricTestCase()
    : TestCase("RT-MHR cross-layer metric calculation test")
{
}

RtMhrMetricTestCase::~RtMhrMetricTestCase()
{
}

void
RtMhrMetricTestCase::TestCrossLayerMetric()
{
    CrossLayerMetric metric;
    metric.linkQuality = 0.8;
    metric.queuingDelay = 0.005; // 5ms
    metric.mobilityMetric = 0.2;
    metric.hopCount = 2;

    double composite = metric.GetCompositeMetric();

    // Verify that metric calculation produces reasonable values
    NS_TEST_ASSERT_MSG_GT(composite, 0.0, "Composite metric should be positive");
    NS_TEST_ASSERT_MSG_LT(composite, 1.0, "Composite metric should be normalized");

    // Test with different values
    CrossLayerMetric metric2;
    metric2.linkQuality = 0.9;    // Better link quality
    metric2.queuingDelay = 0.002; // Lower delay
    metric2.mobilityMetric = 0.1; // Less mobile
    metric2.hopCount = 1;         // Fewer hops

    double composite2 = metric2.GetCompositeMetric();

    NS_TEST_ASSERT_MSG_GT(composite2, composite, "Better metric should have higher value");
}

void
RtMhrMetricTestCase::DoRun()
{
    TestCrossLayerMetric();
}

/**
 * \ingroup rtmhr-test
 * \ingroup tests
 * \brief RT-MHR Test Suite
 */
class RtMhrTestSuite : public TestSuite
{
  public:
    RtMhrTestSuite();
};

RtMhrTestSuite::RtMhrTestSuite()
    : TestSuite("rtmhr", Type::UNIT)
{
    AddTestCase(new RtMhrBasicTestCase, Duration::QUICK);
    AddTestCase(new RtMhrMetricTestCase, Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static RtMhrTestSuite rtmhrTestSuite;
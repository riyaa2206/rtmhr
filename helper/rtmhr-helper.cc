#include "rtmhr-helper.h"

#include "ns3/ipv4-list-routing.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/rtmhr.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RtMhrHelper");

RtMhrHelper::RtMhrHelper()
{
    m_agentFactory.SetTypeId("ns3::RtMhr");
}

RtMhrHelper::~RtMhrHelper()
{
}

RtMhrHelper::RtMhrHelper(const RtMhrHelper& o)
    : m_agentFactory(o.m_agentFactory)
{
}

RtMhrHelper*
RtMhrHelper::Copy() const
{
    return new RtMhrHelper(*this);
}

Ptr<Ipv4RoutingProtocol>
RtMhrHelper::Create(Ptr<Node> node) const
{
    Ptr<RtMhr> agent = m_agentFactory.Create<RtMhr>();
    node->AggregateObject(agent);
    return agent;
}

void
RtMhrHelper::Set(std::string name, const AttributeValue& value)
{
    m_agentFactory.Set(name, value);
}

int64_t
RtMhrHelper::AssignStreams(NodeContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<Node> node;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        node = (*i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");
        Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
        NS_ASSERT_MSG(proto, "Ipv4 routing not installed on node");
        Ptr<RtMhr> rtmhr = DynamicCast<RtMhr>(proto);
        if (rtmhr)
        {
            currentStream += rtmhr->AssignStreams(currentStream);
            continue;
        }
        // RT-MHR may also be in a list
        Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(proto);
        if (list)
        {
            int16_t priority;
            Ptr<Ipv4RoutingProtocol> listProto;
            Ptr<RtMhr> listRtmhr;
            for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++)
            {
                listProto = list->GetRoutingProtocol(i, priority);
                listRtmhr = DynamicCast<RtMhr>(listProto);
                if (listRtmhr)
                {
                    currentStream += listRtmhr->AssignStreams(currentStream);
                    break;
                }
            }
        }
    }
    return (currentStream - stream);
}

void
RtMhrHelper::Install() const
{
    NodeContainer allNodes;
    for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i)
    {
        allNodes.Add(NodeList::GetNode(i));
    }
    Install(allNodes);
}

void
RtMhrHelper::Install(Ptr<Node> node) const
{
    NS_LOG_FUNCTION(this << node->GetId());

    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");

    Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
    if (!proto)
    {
        // No routing protocol installed, install RT-MHR directly
        Ptr<RtMhr> rtmhr = Create(node)->GetObject<RtMhr>();
        ipv4->SetRoutingProtocol(rtmhr);
    }
    else
    {
        // Check if it's already RT-MHR
        Ptr<RtMhr> rtmhr = DynamicCast<RtMhr>(proto);
        if (rtmhr)
        {
            NS_LOG_WARN("RT-MHR already installed on node " << node->GetId());
            return;
        }

        // Check if it's a list routing protocol
        Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(proto);
        if (list)
        {
            // Add RT-MHR to the list with high priority
            Ptr<RtMhr> rtmhr = Create(node)->GetObject<RtMhr>();
            list->AddRoutingProtocol(rtmhr, 10);
        }
        else
        {
            // Replace existing protocol with RT-MHR
            NS_LOG_WARN("Replacing existing routing protocol with RT-MHR on node "
                        << node->GetId());
            Ptr<RtMhr> rtmhr = Create(node)->GetObject<RtMhr>();
            ipv4->SetRoutingProtocol(rtmhr);
        }
    }
}

void
RtMhrHelper::Install(NodeContainer c) const
{
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Install(*i);
    }
}

Ptr<RtMhr>
RtMhrHelper::GetRtMhr(Ptr<Node> node) const
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");

    Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
    if (!proto)
    {
        return 0;
    }

    // Check if it's directly RT-MHR
    Ptr<RtMhr> rtmhr = DynamicCast<RtMhr>(proto);
    if (rtmhr)
    {
        return rtmhr;
    }

    // Check if it's in a list
    Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(proto);
    if (list)
    {
        int16_t priority;
        Ptr<Ipv4RoutingProtocol> listProto;
        for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++)
        {
            listProto = list->GetRoutingProtocol(i, priority);
            rtmhr = DynamicCast<RtMhr>(listProto);
            if (rtmhr)
            {
                return rtmhr;
            }
        }
    }

    return 0;
}

} // namespace ns3

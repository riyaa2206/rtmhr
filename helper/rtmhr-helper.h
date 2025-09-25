#ifndef RTMHR_HELPER_H
#define RTMHR_HELPER_H

#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"
#include "ns3/rtmhr.h"

namespace ns3
{

/**
 * \ingroup rtmhr
 * \brief Helper class that adds RT-MHR routing to nodes.
 */
class RtMhrHelper : public Ipv4RoutingHelper
{
  public:
    /**
     * \brief Create an RtMhrHelper to make life easier for managing RT-MHR routing
     */
    RtMhrHelper();

    /**
     * \brief Destroy an RtMhrHelper
     */
    virtual ~RtMhrHelper();

    /**
     * \brief Create an RtMhrHelper from another previously initialized instance (Copy Constructor).
     * \param o The object to copy from
     */
    RtMhrHelper(const RtMhrHelper& o);

    /**
     * \returns pointer to clone of this RtMhrHelper
     */
    RtMhrHelper* Copy() const override;

    /**
     * \param node the node on which the routing protocol will run
     * \returns a newly-created routing protocol
     */
    virtual Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;

    /**
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set.
     *
     * This method controls the attributes of ns3::RtMhr
     */
    void Set(std::string name, const AttributeValue& value);

    /**
     * \brief Assign a fixed random variable stream number to the random variables used by this
     * model.
     *
     * \param stream first stream index to use
     * \param c NodeContainer of the set of nodes for which the RtMhr model should use a fixed
     * stream
     * \return the number stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

    /**
     * \brief Install RT-MHR routing on all nodes in the container
     * \param c The container of nodes
     */
    void Install() const;

    /**
     * \brief Install RT-MHR routing on a specific node
     * \param node The node to install on
     */
    void Install(Ptr<Node> node) const;

    /**
     * \brief Install RT-MHR routing on all nodes in the container
     * \param c The container of nodes to install on
     */
    void Install(NodeContainer c) const;

    /**
     * \brief Get the RT-MHR routing protocol from a node
     * \param node The node to get the protocol from
     * \return Pointer to the RT-MHR routing protocol
     */
    Ptr<RtMhr> GetRtMhr(Ptr<Node> node) const;

  private:
    ObjectFactory m_agentFactory; ///< Object factory
};

} // namespace ns3

#endif /* RTMHR_HELPER_H */

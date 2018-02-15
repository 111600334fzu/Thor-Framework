/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004 Francisco J. Ros
 * Copyright (c) 2007 INESC Porto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Francisco J. Ros  <fjrm@dif.um.es>
 *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
 */

#ifndef LQ_OLSR_AGENT_IMPL_H
#define LQ_OLSR_AGENT_IMPL_H

#include "lq-olsr-header.h"
#include "lq-olsr-state.h"
#include "lq-olsr-repositories.h"

#include "ns3/lq-metric.h"
#include "ns3/test.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/event-garbage-collector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-static-routing.h"

#include <vector>
#include <map>

 /********** Useful macros **********/

///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (Simulator::Now ())) ? Seconds (0.000001) : \
		     (time - Simulator::Now () + Seconds (0.000001)))

/// Testcase for MPR computation mechanism
class LqOlsrMprTestCase;

namespace ns3 {
namespace lqolsr {

///
/// \defgroup olsr OLSR Routing
/// This section documents the API of the ns-3 OLSR module. For a generic
/// functional description, please refer to the ns-3 manual.

/// \ingroup olsr
/// An %OLSR's routing table entry.
struct RoutingTableEntry
{
  Ipv4Address destAddr; //!< Address of the destination node.
  Ipv4Address nextAddr; //!< Address of the next hop.
  uint32_t interface; //!< Interface index
  uint32_t distance; //!< Distance in hops to the destination.
  float cost; //!< Cost to the destination.

  RoutingTableEntry () : // default values
    destAddr (), nextAddr (),
    interface (0), distance (0), cost(0)
  {
  }
};

/// \ingroup olsr
  /// A Link Tuple.
  struct DestinationTuple
  {
    /// Interface address of the local node.
    Ipv4Address destAddress;
    /// Interface address of the neighbor node.
    const LinkTuple * accessLink;
  };

  static inline bool
  operator == (const DestinationTuple &a, const DestinationTuple &b)
  {
    return (a.destAddress == b.destAddress);
  }

class RoutingProtocol;

///
/// \ingroup olsr
///
/// \brief OLSR routing protocol for IPv4
///
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  friend class ::LqOlsrMprTestCase;

  /**
   * \brief Get the type ID.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  RoutingProtocol ();
  virtual ~RoutingProtocol ();

  lqmetric::LqAbstractMetric::MetricType getMetricType();
  /**
   * \brief Set the OLSR main address to the first address on the indicated interface.
   *
   * \param interface IPv4 interface index
   */
  void SetMainInterface (uint32_t interface);

  /**
   * Dump the neighbor table, two-hop neighbor table, and routing table
   * to logging output (NS_LOG_DEBUG log level).  If logging is disabled,
   * this function does nothing.
   */
  void Dump (void);

  /**
   * Return the list of routing table entries discovered by OLSR
   */
  std::vector<RoutingTableEntry> GetRoutingTableEntries () const;

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  bool HasLinkTo(const Ipv4Address & neighbor);

  /**
   * TracedCallback signature for Packet transmit and receive events.
   *
   * \param [in] header
   * \param [in] messages
   */
  typedef void (* PacketTxRxTracedCallback)(const PacketHeader & header, const MessageList & messages);

  /**
   * TracedCallback signature for routing table computation.
   *
   * \param [in] size Final routing table size.
   */
  typedef void (* TableChangeTracedCallback) (uint32_t size);

private:
  std::set<uint32_t> m_interfaceExclusions; //!< Set of interfaces excluded by OSLR.
  Ptr<Ipv4StaticRouting> m_routingTableAssociation; //!< Associations from an Ipv4StaticRouting instance


public:
  /**
   * Get the excluded interfaces.
   * \returns Container of excluded interfaces.
   */
  std::set<uint32_t> GetInterfaceExclusions () const
  {
    return m_interfaceExclusions;
  }

  /**
   *  \brief Sets the link-quality aware metric used. This method only alters the state of olsr class once. The metric
   *  cannot be reseted
   *
   * \param metric The metric to be set.
   */

  void SetLqMetric(Ptr<lqmetric::LqAbstractMetric> metric);

  /**
   * Set the interfaces to be excluded.
   * \param exceptions Container of excluded interfaces.
   */
  void SetInterfaceExclusions (std::set<uint32_t> exceptions);

  /**
   *  \brief Injects the specified (networkAddr, netmask) tuple in the list of
   *  local HNA associations to be sent by the node via HNA messages.
   *  If this tuple already exists, nothing is done.
   *
   * \param networkAddr The network address.
   * \param netmask The network mask.
   */
  void AddHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask);

  /**
   * \brief Removes the specified (networkAddr, netmask) tuple from the list of
   * local HNA associations to be sent by the node via HNA messages.
   * If this tuple does not exist, nothing is done (see "OlsrState::EraseAssociation()").
   *
   * \param networkAddr The network address.
   * \param netmask The network mask.
   */
  void RemoveHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask);

  /**
   * \brief Associates the specified Ipv4StaticRouting routing table
   *         to the OLSR routing protocol. Entries from this associated
   *         routing table that use non-olsr outgoing interfaces are added
   *         to the list of local HNA associations so that they are included
   *         in HNA messages sent by the node.
   *         If this method is called more than once, entries from the old
   *         association are deleted before entries from the new one are added.
   *  \param routingTable the Ipv4StaticRouting routing table to be associated.
   */
  void SetRoutingTableAssociation (Ptr<Ipv4StaticRouting> routingTable);

  /**
   * \brief Returns the internal HNA table
   * \returns the internal HNA table
   */
  Ptr<const Ipv4StaticRouting> GetRoutingTableAssociation () const;

protected:
  virtual void DoInitialize (void);
  Ptr<ns3::lqmetric::LqAbstractMetric> m_metric;
  LqOlsrState m_state;  //!< Internal state with all needed data structs.
  EventGarbageCollector m_events; //!< Running events.

  // From Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput (Ptr<const Packet> p,
                             const Ipv4Header &header,
                             Ptr<const NetDevice> idev,
                             UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb,
                             LocalDeliverCallback lcb,
                             ErrorCallback ecb);

  Ipv4Address GetMyMainAddress();

  /**
     * \brief Removes association tuple_ if expired. Else timer is rescheduled to expire at tuple_->time().
     *
     * \param gatewayAddr The gateway address.
     * \param networkAddr The network address.
     * \param netmask  The network mask.
     */
    virtual void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);

  /**
   *
   * \brief Processes a HNA message following \RFC{3626} specification.
   *
   * The Host Network Association Set is updated (if needed) with the information
   * of the received HNA message.
   *
   * \param msg the %OLSR message which contains the HNA message.
   * \param senderIface the address of the interface where the message was sent from.
   *
   */
  virtual void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);


  /**
   * \brief Creates the routing table of the node following \RFC{3626} hints.
   */
  virtual void RoutingTableComputation ();

  /**
     * \brief Looks up an entry for the specified destination address.
     * \param [in] dest Destination address.
     * \param [out] outEntry Holds the routing entry result, if found.
     * \return true if found, false if not found.
     */
    bool Lookup (const Ipv4Address &dest,
                 RoutingTableEntry &outEntry) const;


private:
  std::map<Ipv4Address, RoutingTableEntry> m_table; //!< Data structure for the routing table.
  std::vector<DestinationTuple> m_destinations;
  std::map<Ipv4Address, float> m_costs;

  Ptr<Ipv4StaticRouting> m_hnaRoutingTable; //!< Routing table for HNA routes

  uint16_t m_packetSequenceNumber;    //!< Packets sequence number counter.
  uint16_t m_messageSequenceNumber;   //!< Messages sequence number counter.
  uint16_t m_ansn;  //!< Advertised Neighbor Set sequence number.

  Time m_helloInterval;   //!< HELLO messages' emission interval.
  Time m_tcInterval;      //!< TC messages' emission interval.
  Time m_midInterval;     //!< MID messages' emission interval.
  Time m_hnaInterval;     //!< HNA messages' emission interval.
  uint8_t m_willingness;  //!<  Willingness for forwarding packets on behalf of other nodes.
  Ptr<Ipv4> m_ipv4;   //!< IPv4 object the routing is linked to.

  /**
   * \brief Clears the routing table and frees the memory assigned to each one of its entries.
   */
  void Clear ();

  /**
   * Returns the routing table size.
   * \return The routing table size.
   */
  uint32_t GetSize () const
  {
    return m_table.size ();
  }

  /**
   * \brief Deletes the entry whose destination address is given.
   * \param dest address of the destination node.
   */
  void RemoveEntry (const Ipv4Address &dest);

  MessageList FilterNonDuplicatedMessages(const MessageList & messages);

  void ProcessNonDuplicatedMessage(const MessageHeader & messageHeader, const Ipv4Address & senderIfaceAddr, const Ipv4Address & receiverIfaceAddr);

  void ProcessMessage(const MessageList & messages, const Ipv4Address & senderIfaceAddr, const Ipv4Address & receiverIfaceAddr, const InetSocketAddress & inetSourceAddr);

  MessageList ExtractCorrectMessagesFromPacket(Ptr<Packet> packet, const lqolsr::PacketHeader & olsrPacketHeader);

  uint32_t
  GetInterfaceNumberByAddress(const Ipv4Address & intAddress);

  RoutingTableEntry
  CreateLqEntry (Ipv4Address const &dest, Ipv4Address const &next, Ipv4Address const &interfaceAddress, float cost);

  /**
   * \brief Adds a new entry into the routing table.
   *
   * If an entry for the given destination existed, it is deleted and freed.
   *
   * \param dest address of the destination node.
   * \param next address of the next hop node.
   * \param interface address of the local interface.
   * \param cost cost to the destination node.
   */
    void AddLqEntry (const Ipv4Address &dest,
                   const Ipv4Address &next,
                   uint32_t interface,
                   float cost);

  /**
   * \brief Adds a new entry into the routing table.
   *
   * If an entry for the given destination existed, it is deleted and freed.
   *
   * \param dest address of the destination node.
   * \param next address of the next hop node.
   * \param interface address of the local interface.
   * \param distance distance to the destination node.
   */
  void AddEntry (const Ipv4Address &dest,
                 const Ipv4Address &next,
                 uint32_t interface,
                 uint32_t distance);

  /**
   * \brief Adds a new entry into the routing table.
   *
   * If an entry for the given destination existed, an error is thrown.
   *
   * \param dest address of the destination node.
   * \param next address of the next hop node.
   * \param interfaceAddress address of the local interface.
   * \param cost cost to the destination node.
   */
    void AddLqEntry (const Ipv4Address &dest,
                   const Ipv4Address &next,
                   const Ipv4Address &interfaceAddress,
                   float cost);

  /**
   * \brief Adds a new entry into the routing table.
   *
   * If an entry for the given destination existed, an error is thrown.
   *
   * \param dest address of the destination node.
   * \param next address of the next hop node.
   * \param interfaceAddress address of the local interface.
   * \param distance distance to the destination node.
   */
  void AddEntry (const Ipv4Address &dest,
                 const Ipv4Address &next,
                 const Ipv4Address &interfaceAddress,
                 uint32_t distance);

  bool
  UpdateEntry(Ipv4Address const &dest, Ipv4Address const &next, const Ipv4Address & interfaceAddress, float cost);

  /**
   * \brief Finds the appropriate entry which must be used in order to forward
   * a data packet to a next hop (given a destination).
   *
   * Imagine a routing table like this: [A,B] [B,C] [C,C]; being each pair of the
   * form [dest addr, next-hop addr]. In this case, if this function is invoked
   * with [A,B] then pair [C,C] is returned because C is the next hop that must be used
   * to forward a data packet destined to A. That is, C is a neighbor of this node,
   * but B isn't. This function finds the appropriate neighbor for forwarding a packet.
   *
   * \param[in] entry The routing table entry which indicates the destination node
   * we are interested in.
   *
   * \param[out] outEntry The appropriate routing table entry which indicates the next
   * hop which must be used for forwarding a data packet, or NULL if there is no such entry.
   *
   * \return True if an entry was found, false otherwise.
   */
  bool FindSendEntry (const RoutingTableEntry &entry,
                      RoutingTableEntry &outEntry) const;


  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

  void DoDispose ();

  /**
   * Send an OLSR message.
   * \param packet The packet to be sent.
   * \param containedMessages The messages contained in the packet.
   */
  void SendPacket (Ptr<Packet> packet, const MessageList &containedMessages);

  /**
   * Increments packet sequence number and returns the new value.
   * \return The packet sequence number.
   */
  inline uint16_t GetPacketSequenceNumber ();

  /**
   * Increments message sequence number and returns the new value.
   * \return The message sequence number.
   */
  inline uint16_t GetMessageSequenceNumber ();

  /**
   * Receive an OLSR message.
   * \param socket The receiving socket.
   */
  void RecvOlsr (Ptr<Socket> socket);

  /**
   * \brief Computates MPR set of a node considering link quality.
   */
  void
  MprComputation ();

  void
  InitializeDestinations();

  int
  GetMinDestination();

  void
  UpdateDestinationNeighbors(const DestinationTuple & dest);


  /**
   * \brief Include multiple associated interfaces into the routing table
   */
  void
  AddInterfaceAssociationsToRoutingTable();

  /**
   * \brief Include routes for wxternal networks
   */
  void
  CalculateHNARoutingTable();


  /**
   * \brief Gets the main address associated with a given interface address.
   * \param iface_addr the interface address.
   * \return the corresponding main address.
   */
  Ipv4Address GetMainAddress (Ipv4Address iface_addr) const;

  /**
   *  \brief Tests whether or not the specified route uses a non-OLSR outgoing interface.
   *  \param route The route to be tested.
   *  \returns True if the outgoing interface of the specified route is a non-OLSR interface, false otherwise.
   */
  bool UsesNonOlsrOutgoingInterface (const Ipv4RoutingTableEntry &route);

  // Timer handlers
  Timer m_helloTimer; //!< Timer for the HELLO message.
  /**
   * \brief Sends a HELLO message and reschedules the HELLO timer.
   */
  void HelloTimerExpire ();

  Timer m_tcTimer; //!< Timer for the TC message.
  /**
   * \brief Sends a TC message (if there exists any MPR selector) and reschedules the TC timer.
   */
  void TcTimerExpire ();

  Timer m_midTimer; //!< Timer for the MID message.
  /**
   * \brief \brief Sends a MID message (if the node has more than one interface) and resets the MID timer.
   */
  void MidTimerExpire ();

  Timer m_hnaTimer; //!< Timer for the HNA message.
  /**
   * \brief Sends an HNA message (if the node has associated hosts/networks) and reschedules the HNA timer.
   */
  void HnaTimerExpire ();

  /**
   * \brief Removes tuple if expired. Else timer is rescheduled to expire at tuple.expirationTime.
   *
   * The task of actually removing the tuple is left to the OLSR agent.
   *
   * \param address The address of the tuple.
   * \param sequenceNumber The sequence number of the tuple.
   */
  void DupTupleTimerExpire (Ipv4Address address, uint16_t sequenceNumber);

  bool m_linkTupleTimerFirstTime; //!< Flag to indicate if it is the first time the LinkTupleTimer fires.
  /**
   * \brief Removes tuple_ if expired. Else if symmetric time
   * has expired then it is assumed a neighbor loss and agent_->nb_loss()
   * is called. In this case the timer is rescheduled to expire at
   * tuple_->time(). Otherwise the timer is rescheduled to expire at
   * the minimum between tuple_->time() and tuple_->sym_time().
   *
   * The task of actually removing the tuple is left to the OLSR agent.
   *
   * \param neighborIfaceAddr The tuple neighbor interface address.
   */
  void LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr);

  /**
   * \brief Removes 2_hop neighbor tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
   *
   * The task of actually removing the tuple is left to the OLSR agent.
   *
   * \param neighborMainAddr The neighbor main address.
   * \param twoHopNeighborAddr The 2-hop neighbor address.
   */
  void Nb2hopTupleTimerExpire (Ipv4Address neighborMainAddr, Ipv4Address twoHopNeighborAddr);

  /**
   * \brief Removes MPR selector tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
   *
   * The task of actually removing the tuple is left to the OLSR agent.
   *
   * \param mainAddr The tuple IPv4 address.
   */
  void MprSelTupleTimerExpire (Ipv4Address mainAddr);

  /**
   * \brief Removes topology tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
   *
   * The task of actually removing the tuple is left to the OLSR agent.
   *
   * \param destAddr The destination address.
   * \param lastAddr The last address.
   */
  void TopologyTupleTimerExpire (Ipv4Address destAddr, Ipv4Address lastAddr);

  /**
   * \brief Removes interface association tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
   *
   * \param ifaceAddr The interface address.
   */
  void IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr);

  /**
   * Increments the ANSN counter.
   */
  void IncrementAnsn ();

  /// A list of pending messages which are buffered awaiting for being sent.
  lqolsr::MessageList m_queuedMessages;
  Timer m_queuedMessagesTimer; //!< timer for throttling outgoing messages

  /**
   * \brief OLSR's default forwarding algorithm.
   *
   * See \RFC{3626} for details.
   *
   * \param olsrMessage The %OLSR message which must be forwarded.
   * \param duplicated NULL if the message has never been considered for forwarding, or a duplicate tuple in other case.
   * \param localIface The address of the interface where the message was received from.
   * \param senderAddress The sender IPv4 address.
   */
  void ForwardDefault (lqolsr::MessageHeader olsrMessage,
                       DuplicateTuple *duplicated,
                       const Ipv4Address &localIface,
                       const Ipv4Address &senderAddress);

  /**
   * \brief Enques an %OLSR message which will be sent with a delay of (0, delay].
   *
   * This buffering system is used in order to piggyback several %OLSR messages in
   * a same %OLSR packet.
   *
   * \param message the %OLSR message which must be sent.
   * \param delay maximum delay the %OLSR message is going to be buffered.
   */
  void QueueMessage (const lqolsr::MessageHeader &message, Time delay);

  /**
   * \brief Creates as many %OLSR packets as needed in order to send all buffered
   * %OLSR messages.
   *
   * Maximum number of messages which can be contained in an %OLSR packet is
   * dictated by OLSR_MAX_MSGS constant.
   */
  void SendQueuedMessages ();

  /**
   * \brief Creates a new %OLSR HELLO message which is buffered for being sent later on.
   */
  void SendHello ();

  /**
   * \brief Creates a new %OLSR TC message which is buffered for being sent later on.
   */
  void SendTc ();

  /**
   * \brief Creates a new %OLSR MID message which is buffered for being sent later on.
   */
  void SendMid ();

  /**
   * \brief Creates a new %OLSR HNA message which is buffered for being sent later on.
   */
  void SendHna ();

  /**
   * \brief Get the link code for a link message to be included in a Hello or LqHello packet.
   * \param link_tuple The LinkTuple to be analysed
   * \param the simulator time representing the current moment
   */
  uint8_t
  GetLinkCode(const LinkTuple & link_tuple, Time now);

  /*
   *\brief Creates a new message to send.
   *\param vTime the VTime of the message
   *\param timeToLive the Time to Live of the message
   */
  lqolsr::MessageHeader
  CreateNewMessage(Time vTime, uint8_t timeToLive);

  /**
   * \brief Performs all actions needed when a neighbor loss occurs.
   *
   * Neighbor Set, 2-hop Neighbor Set, MPR Set and MPR Selector Set are updated.
   *
   * \param tuple link tuple with the information of the link to the neighbor which has been lost.
   */
  void NeighborLoss (const LinkTuple &tuple);

  /**
   * \brief Adds a duplicate tuple to the Duplicate Set.
   *
   * \param tuple The duplicate tuple to be added.
   */
  void AddDuplicateTuple (const DuplicateTuple &tuple);

  /**
   * \brief Removes a duplicate tuple from the Duplicate Set.
   *
   * \param tuple The duplicate tuple to be removed.
   */
  void RemoveDuplicateTuple (const DuplicateTuple &tuple);

  /**
   * Adds a link tuple.
   * \param tuple Thetuple to be added.
   * \param willingness The tuple willingness.
   */
  void LinkTupleAdded (const LinkTuple &tuple, uint8_t willingness);

  /**
   * \brief Removes a link tuple from the Link Set.
   *
   * \param tuple The link tuple to be removed.
   */
  void RemoveLinkTuple (const LinkTuple &tuple);

  /**
   * \brief This function is invoked when a link tuple is updated. Its aim is to
   * also update the corresponding neighbor tuple if it is needed.
   *
   * \param tuple The link tuple which has been updated.
   * \param willingness The tuple willingness.
   */
  void LinkTupleUpdated (const LinkTuple &tuple, uint8_t willingness);

  /**
   * \brief Adds a neighbor tuple to the Neighbor Set.
   *
   * \param tuple The neighbor tuple to be added.
   */
  void AddNeighborTuple (const NeighborTuple &tuple);

  /**
   * \brief Removes a neighbor tuple from the Neighbor Set.
   *
   * \param tuple The neighbor tuple to be removed.
   */
  void RemoveNeighborTuple (const NeighborTuple &tuple);

  /**
   * \brief Adds a 2-hop neighbor tuple to the 2-hop Neighbor Set.
   *
   * \param tuple The 2-hop neighbor tuple to be added.
   */
  void AddTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple);

  /**
   * \brief Removes a 2-hop neighbor tuple from the 2-hop Neighbor Set.
   *
   * \param tuple The 2-hop neighbor tuple to be removed.
   */
  void RemoveTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple);

  /**
   * \brief Adds an MPR selector tuple to the MPR Selector Set.
   * Advertised Neighbor Sequence Number (ANSN) is also updated.
   *
   * \param tuple The MPR selector tuple to be added.
   */
  void AddMprSelectorTuple (const MprSelectorTuple  &tuple);

  /**
   * \brief Removes an MPR selector tuple from the MPR Selector Set.
   * Advertised Neighbor Sequence Number (ANSN) is also updated.
   *
   * \param tuple The MPR selector tuple to be removed.
   */
  void RemoveMprSelectorTuple (const MprSelectorTuple &tuple);

  /**
   * \brief Adds a topology tuple to the Topology Set.
   *
   * \param tuple The topology tuple to be added.
   */
  void AddTopologyTuple (const TopologyTuple &tuple);

  /**
   * \brief Removes a topology tuple to the Topology Set.
   *
   * \param tuple The topology tuple to be removed.
   */
  void RemoveTopologyTuple (const TopologyTuple &tuple);

  /**
   * \brief Adds an interface association tuple to the Interface Association Set.
   *
   * \param tuple The interface association tuple to be added.
   */
  void AddIfaceAssocTuple (const IfaceAssocTuple &tuple);

  /**
   * \brief Removed an interface association tuple to the Interface Association Set.
   *
   * \param tuple The interface association tuple to be removed.
   */
  void RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple);

  /**
   * \brief Adds a host network association tuple to the Association Set.
   *
   * \param tuple The host network association tuple to be added.
   */
  void AddAssociationTuple (const AssociationTuple &tuple);

  /**
   * \brief Removes a host network association tuple to the Association Set.
   *
   * \param tuple The host network association tuple to be removed.
   */
  void RemoveAssociationTuple (const AssociationTuple &tuple);

  /**
   * \brief Processes a HELLO message following \RFC{3626} specification.
   *
   * Link sensing and population of the Neighbor Set, 2-hop Neighbor Set and MPR
   * Selector Set are performed.
   *
   * \param msg the %OLSR message which contains the HELLO message.
   * \param receiverIface the address of the interface where the message was received from.
   * \param senderIface the address of the interface where the message was sent from.
   */
  void ProcessHello (const lqolsr::MessageHeader &msg,
                     const Ipv4Address &receiverIface,
                     const Ipv4Address &senderIface);

  /**
   * \brief Creates a new topology tuple
   * \param tuple The topology tuple to be created
   * \param destAddress The destination address
   * \param lastAddress The address of the node just before the destination node
   */
  void CreateTopologyTuple(TopologyTuple & tuple, const Ipv4Address & destAddress,
			   const Ipv4Address & lastAddress, uint16_t seqNumber, Time expirationTime, float cost);

  /**
   * \brief Processes a TC message following \RFC{3626} specification.
   *
   * The Topology Set is updated (if needed) with the information of
   * the received TC message.
   *
   * \param msg The %OLSR message which contains the TC message.
   * \param senderIface The address of the interface where the message was sent from.
   *
   */
  void ProcessTc (const lqolsr::MessageHeader &msg,
                  const Ipv4Address &senderIface);

  /**
   * \brief Processes a MID message following \RFC{3626} specification.
   *
   * The Interface Association Set is updated (if needed) with the information
   * of the received MID message.
   *
   * \param msg the %OLSR message which contains the MID message.
   * \param senderIface the address of the interface where the message was sent from.
   */
  void ProcessMid (const lqolsr::MessageHeader &msg,
                   const Ipv4Address &senderIface);

  /**
   * \brief Creates a new link tuple.
   * \param senderIface the sender's interface address
   * \param receiverIface the receiver's interface address
   * \param now the current time
   * \param vtime the vTime in the message
   */
  void
  CreateNewLinkTuple(LinkTuple &newLinkTuple, const Ipv4Address &senderIface, const Ipv4Address &receiverIface, Time now, Time vtime);

  /**
   * \brif Log (if enabled) LinkSensing information
   * \param linkType The type of the link
   * \param neighborType The type of the neighbor
   */
  void
  LogLinkSensing(int linkType, int neighborType);

  bool
  ProcessLqHelloLinkMessages(LinkTuple *link_tuple,  const lqolsr::MessageHeader::LqHello &lqhello,
  			   const Ipv4Address &receiverIface, Time now, Time vTime);



  /**
   * \brief Updates Link Set according to a new received HELLO message
   * (following \RFC{3626} specification). Neighbor Set is also updated if needed.
   * \param msg The received message.
   * \param hello The received HELLO sub-message.
   * \param receiverIface The interface that received the message.
   * \param senderIface The sender interface.
   */
  void LinkSensing (const lqolsr::MessageHeader &msg,
                    const lqolsr::MessageHeader::LqHello &hello,
                    const Ipv4Address &receiverIface,
                    const Ipv4Address &senderIface);

  /**
   * \brief Updates the Neighbor Set according to the information contained in
   * a new received HELLO message (following \RFC{3626}).
   * \param msg The received message.
   * \param hello The received HELLO sub-message.
   */
  void PopulateNeighborSet (const lqolsr::MessageHeader &msg,
                            const lqolsr::MessageHeader::LqHello &hello);

  /**
   * \brief Shows log information for the PopulateTwoHopNeighborSet and PopulateTwoHopNeighborSetLq methods
   * \param neighborType the neighbor type of a LinkMessage
   */
  void LogPopulateTwoHopNeighborSet(int neighborType);


  /**
   * \brief Updates the 2-hop Neighbor Set according to the information contained
   * in a new received LQ_HELLO message. Works only when link quality is enabled.
   * \param msg The received message.
   * \param lqhello The received LQ_HELLO sub-message.
   */
    void PopulateTwoHopNeighborSetLq (const lqolsr::MessageHeader &msg,
                                    const lqolsr::MessageHeader::LqHello &lqhello);

   /**
   * \brief Updates the MPR Selector Set according to the information contained in
   * a new received HELLO message (following \RFC{3626}).
   * \param msg The received message.
   * \param hello The received HELLO sub-message.
   */
  void PopulateMprSelectorSet (const lqolsr::MessageHeader &msg,
                               const lqolsr::MessageHeader::LqHello &hello);


  /// Check that address is one of my interfaces
  bool IsMyOwnAddress (const Ipv4Address & a) const;

  Ipv4Address m_mainAddress; //!< the node main address.

  // One socket per interface, each bound to that interface's address
  // (reason: for OLSR Link Sensing we need to know on which interface
  // HELLO messages arrive)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses; //!< Container of sockets and the interfaces they are opened onto.

  /// Rx packet trace.
  TracedCallback <const PacketHeader &, const MessageList &> m_rxPacketTrace;

  /// Tx packet trace.
  TracedCallback <const PacketHeader &, const MessageList &> m_txPacketTrace;

  /// Routing table chanes challback
  TracedCallback <uint32_t> m_routingTableChanged;

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;

};

}
}  // namespace ns3

#endif /* OLSR_AGENT_IMPL_H */

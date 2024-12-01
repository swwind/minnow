#include <cstdio>
#include <iostream>
#include <utility>

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "exception.hh"
#include "ipv4_datagram.hh"
#include "network_interface.hh"
#include "parser.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto& state = arp_[next_hop.ipv4_numeric()];
  // if not found
  if ( !state.found ) {
    if ( state.request <= timestamp_ ) {
      // send a request
      ARPMessage arp;
      arp.opcode = ARPMessage::OPCODE_REQUEST;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.sender_ethernet_address = ethernet_address_;
      arp.target_ip_address = next_hop.ipv4_numeric();
      send_arp( ETHERNET_BROADCAST, arp );
      state.request = timestamp_ + 5000;
    }
    // fprintf( stderr, "request is %u\n", (unsigned int)state.request );
    state.cache.push_back( dgram );
    arp_[next_hop.ipv4_numeric()] = state;
  } else {
    send_ipv4( state.eth_address, dgram );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // if is ARP
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage message;
    if ( parse( message, frame.payload ) ) {
      update_arp( message.sender_ip_address, message.sender_ethernet_address );
      if ( message.opcode == ARPMessage::OPCODE_REQUEST
           && message.target_ip_address == ip_address_.ipv4_numeric() ) {
        ARPMessage arp;
        arp.opcode = ARPMessage::OPCODE_REPLY;
        arp.sender_ip_address = ip_address_.ipv4_numeric();
        arp.sender_ethernet_address = ethernet_address_;
        arp.target_ip_address = message.sender_ip_address;
        arp.target_ethernet_address = message.sender_ethernet_address;
        send_arp( message.sender_ethernet_address, arp );
      }
    }
  }
  // if is IPv4
  else if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    if ( frame.header.dst == ethernet_address_ || frame.header.dst == ETHERNET_BROADCAST ) {
      InternetDatagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        datagrams_received_.push( dgram );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timestamp_ += ms_since_last_tick;
  std::vector<uint32_t> forget;
  for ( auto p : arp_ ) {
    if ( p.second.found && p.second.expire <= timestamp_ ) {
      forget.push_back( p.first );
    }
  }
  for ( auto v : forget ) {
    arp_[v] = ARPState {};
  }
}

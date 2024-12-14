#include "router.hh"
#include "address.hh"

#include <algorithm>
#include <cstdint>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  uint32_t subnetmask = prefix_length ? ~( ( 1u << ( 32 - prefix_length ) ) - 1u ) : 0;
  _route_table.emplace_back( route_prefix & subnetmask, subnetmask, next_hop, interface_num );
  dirty = true;
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  if ( dirty ) {
    sort( _route_table.begin(), _route_table.end(), [&]( const Route& a, const Route& b ) {
      return a.subnet_mask > b.subnet_mask;
    } );
    dirty = false;
  }

  for ( auto& i : _interfaces ) {
    auto& q = i->datagrams_received();
    while ( !q.empty() ) {
      auto dgram = q.front();
      q.pop();

      if ( !dgram.header.ttl )
        continue;
      if ( !--dgram.header.ttl )
        continue;
      dgram.header.compute_checksum();

      for ( auto& route : _route_table ) {
        if ( ( dgram.header.dst & route.subnet_mask ) == route.route_prefix ) {
          _interfaces[route.interface_num]->send_datagram(
            dgram, route.next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) ) );
          break;
        }
      }
    }
  }
}

#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <cstdio>
#include <optional>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
  } else if ( message.SYN ) {
    zero_point_ = message.seqno;
    reassembler_.insert( 0, message.payload, message.FIN );
  } else if ( zero_point_.has_value() ) {
    // fprintf( stderr, "length = %lu\n", message.payload.length() );
    // auto seq = message.seqno.unwrap( zero_point_.value(), reassembler_.writer().bytes_pushed() );
    // fprintf( stderr, "seq = %lu\n", seq );
    reassembler_.insert( message.seqno.unwrap( zero_point_.value(), reassembler_.writer().bytes_pushed() ) - 1,
                         message.payload,
                         message.FIN );
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  auto& w = reassembler_.writer();
  // fprintf( stderr, "bytes_pushed = %lu\n", w.bytes_pushed() );
  return TCPReceiverMessage {
    zero_point_.has_value()
      ? (std::optional<Wrap32>)( Wrap32 { 0 }.wrap( w.bytes_pushed() + 1 + w.is_closed(), zero_point_.value() ) )
      : std::nullopt,
    static_cast<uint16_t>( std::min( w.available_capacity(), (uint64_t)UINT16_MAX ) ),
    reassembler_.writer().has_error(),
  };
}

#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <cstdio>
#include <utility>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sent_bytes_ - acked_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retry_count_;
}

#define unsigned_minus( a, b ) ( ( b ) > ( a ) ) ? ( ( a ) = 0 ) : ( ( a ) -= ( b ) )

void TCPSender::push( const TransmitFunction& transmit )
{
  auto& reader = writer().reader();

  while ( true ) {

    auto message = make_empty_message();
    if ( sent_bytes_ == 0 )
      message.SYN = 1;

    auto window = max( receiver_window_, 1ul );
    unsigned_minus( window, sequence_numbers_in_flight() );
    auto payload_window = min( window, TCPConfig::MAX_PAYLOAD_SIZE );
    unsigned_minus( payload_window, sent_bytes_ == 0 );

    auto buffer = reader.peek().substr( 0, payload_window );
    message.payload = buffer;
    reader.pop( buffer.size() );

    if ( buffer.size() < window && reader.is_finished() && !fin_sent_ ) {
      fin_sent_ = true;
      message.FIN = 1;
    }

    if ( message.sequence_length() > 0 ) {
      transmit( message );
      sent_bytes_ += buffer.size() + message.SYN + message.FIN;
      // prepare for resending
      sending_messages_.push( make_pair( sent_bytes_, message ) );
    } else {
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  auto message = TCPSenderMessage {};
  message.seqno = Wrap32::wrap( sent_bytes_, isn_ );
  message.RST = writer().has_error();
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  receiver_window_ = msg.window_size;

  if ( msg.RST ) {
    writer().set_error();
  }

  if ( msg.ackno.has_value() ) {
    auto new_acked_bytes = msg.ackno->unwrap( isn_, acked_bytes_ );

    if ( sending_messages_.empty() || new_acked_bytes > sending_messages_.back().first ) {
      // ignore impossible ack
      return;
    }

    if ( new_acked_bytes > acked_bytes_ ) {
      current_RTO_ms_ = 0;
      retry_count_ = 0;
      acked_bytes_ = new_acked_bytes;

      while ( !sending_messages_.empty() && acked_bytes_ >= sending_messages_.front().first ) {
        sending_messages_.pop();
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{

  // 超时重传
  if ( !sending_messages_.empty() ) {

    if ( ( current_RTO_ms_ += ms_since_last_tick )
         >= ( initial_RTO_ms_ << ( receiver_window_ > 0 ? retry_count_ : 0 ) ) ) {
      transmit( sending_messages_.front().second );
      current_RTO_ms_ = 0;
      retry_count_ += 1;
      return;
    }
  }

  // // 否则什么都不做
}

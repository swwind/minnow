#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return done_;
}

void Writer::push( string data )
{
  auto capacity = available_capacity();
  if ( data.length() > capacity ) {
    data.resize( capacity );
  }
  count_ += data.length();
  buffer_ += data;
}

void Writer::close()
{
  done_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.length();
}

uint64_t Writer::bytes_pushed() const
{
  return count_;
}

bool Reader::is_finished() const
{
  return done_ && !buffer_.length();
}

uint64_t Reader::bytes_popped() const
{
  return count_ - buffer_.length();
}

string_view Reader::peek() const
{
  return string_view( buffer_ );
}

void Reader::pop( uint64_t len )
{
  buffer_.erase( 0, len );
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.length();
}

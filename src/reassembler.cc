#include "reassembler.hh"
#include <vector>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring )
    last_ = first_index + data.length();

  const auto start = min( last_, max( first_index, current_ ) );
  const auto end = min( last_, min( first_index + data.length(), current_ + capacity_ ) );

  // fprintf( stderr, "start, end = %lu, %lu\n", start, end );

  // skip if there is thing to add
  if ( start < end ) {

    // find the first it with start < it->first
    auto it = buffer_slots_.upper_bound( make_pair( start, start ) );
    if ( it != buffer_slots_.begin() )
      --it;

    vector<pair<int, int>> pr;
    while ( it != buffer_slots_.end() ) {
      if ( start >= it->second ) {
        ++it;
        continue;
      }
      if ( end <= it->first )
        break;

      const auto st = max( start, it->first ), ed = min( end, it->second );
      buffer_.push( make_pair( st, data.substr( st - first_index, ed - st ) ) );
      pending_ += ed - st;
      fprintf( stderr, "pushed (%lu, %lu)\n", st, ed );

      if ( it->first < start )
        pr.emplace_back( it->first, start );
      if ( end < it->second )
        pr.emplace_back( end, it->second );

      buffer_slots_.erase( it++ );
    }
    for ( auto p : pr )
      buffer_slots_.insert( p );
  }

  auto& w = reader().writer();
  uint64_t a = 0;
  while ( !buffer_.empty() && buffer_.top().first == current_ && ( ( a = w.available_capacity() ) > 0 ) ) {
    auto top = buffer_.top();
    buffer_.pop();
    const auto size = min( a, top.second.length() );
    // fprintf( stderr, "seems we can push %lu size from %lu\n", size, top.first );
    if ( size == top.second.length() ) {
      w.push( top.second );
    } else {
      w.push( top.second.substr( 0, size ) );
      buffer_.push( make_pair( current_ + size, top.second.substr( size ) ) );
    }
    const auto st = min( current_ + capacity_, last_ );
    const auto ed = min( current_ + capacity_ + size, last_ );
    if ( st < ed )
      buffer_slots_.insert( make_pair( st, ed ) );
    current_ += size;
    pending_ -= size;
  }

  if ( current_ == last_ )
    w.close();
}

uint64_t Reassembler::bytes_pending() const
{
  return pending_ - ( buffer_slots_.empty() ? pending_ : buffer_slots_.begin()->first - current_ );
}

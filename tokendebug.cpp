// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "tokens.hpp"
#include <iterator>

namespace Tokens {
::std::ostream &operator <<(::std::ostream &os, Tokens::AnyToken const &a)
{
   ::std::format_to(::std::ostream_iterator<char>(os), "{}", a);
   return os;
}
} // namespace Tokens

// Copyright © 2025 Eric Hopper. All rights reserved.
// Created on 2025-06-29.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include <ctre.hpp>
#include <iostream>
#include "ring_buffer_view.hpp"
#include <iterator>
#include <vector>
#include <string>

static constexpr auto lex_patterns = ctll::fixed_string{
   "\\s*(?:"
   "(?<int_hex>0x[0-9a-fA-F]+)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_dec>(?:[1-9][0-9]*)|0)"
   ")"
};

auto const tokenizer_re = ::ctre::tokenize<
   lex_patterns
>;

template <::std::forward_iterator I>
::std::vector<::std::string> tokenize_tokenizer(I begin, I end)
{
   ::std::vector<::std::string> result;
   for (auto token: tokenizer_re(begin, end)) {
      if (token) {
         ::std::clog << "got one\n";
         auto const &full_capture = token.template get<0>();
         result.emplace_back(full_capture.begin(), full_capture.end());
      }
   }
   return result;
}

int main()
{
   using rawchars_t = ::std::istreambuf_iterator<char>;
   auto tokenize = [](){
      input_to_forward_range_adapter adapter{
         rawchars_t{::std::cin}, rawchars_t{}
      };
      return tokenize_tokenizer(adapter.begin(), adapter.end());
   };
   auto const tokens = tokenize();
   for (auto const &token: tokens) {
      ::std::cout << " - [\"" << token << "\"]\n";
   }

}

// Copyright © 2025 Eric Hopper. All rights reserved.
// Created on 2025-06-29.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include <ctre.hpp>
#include <iostream>
#include "ring_buffer_view.hpp"
#include <iterator>
#include <vector>
#include <string>
#include <regex>

static constexpr auto lex_patterns = ctll::fixed_string{
   "\\s*(?:"
   "(?<int_dec>(?:[1-9][0-9]*)|0)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_hex>0x[0-9a-fA-F]+)"
   ")\\s+"
};

auto const tokenizer_re = ::ctre::tokenize<
   lex_patterns
>;

auto const startswith_re = ::ctre::starts_with<
   lex_patterns
>;

namespace rexc = ::std::regex_constants;

auto const tokenizer_cppre = ::std::regex{
   ::std::string{lex_patterns.begin(), lex_patterns.end()},
   rexc::ECMAScript | rexc::multiline
};

int main()
{
   int i;
   ::std::istreambuf_iterator<char> it(::std::cin);
   ::std::istreambuf_iterator<char> end;
   input_to_forward_range_adapter adapter{it, end};
#if 0
   using iter_t = decltype(adapter)::iterator;
   iter_t finger = adapter.begin();
   iter_t finished = adapter.end();
   iter_t first_nonspace = finger;
   enum { In_Space, In_NonSpace } state = In_Space;
   ::std::vector<::std::string> words;
   for (; finger != finished; ++finger) {
      if (isspace(*finger)) {
         if (state == In_NonSpace) {
            words.emplace_back(first_nonspace, finger);
         }
         state = In_Space;
      } else {
         if (state == In_Space) {
            first_nonspace = finger;
            state = In_NonSpace;
         }
      }
   }
   if (state == In_NonSpace) {
      words.emplace_back(first_nonspace, finished);
   }
   for (auto const &word: words) {
      ::std::cout << " - [" << word << "]\n";
   }
#else
   for (auto token: tokenizer_re(adapter.begin(), adapter.end())) {
      if (token) {
         ::std::cout << "got one\n";
         const ::std::string value = ::std::string{token.get<0>().begin(), token.get<0>().end()};
         ::std::cout << "[" << value << "]\n";
      }
   }
#endif
}

// Copyright © 2025 Eric Hopper. All rights reserved.
// Created on 2025-06-29.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include <ctre.hpp>
#include <iostream>
#include "ring_buffer_view.hpp"
#include <variant>
#include <iterator>
#include <vector>
#include <string>
#include <format>

static constexpr auto lex_patterns = ctll::fixed_string{
   "(?m)\\s*(?:"
   "(?<int_hex>0x[0-9a-fA-F]+)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_dec>(?:[1-9][0-9]*)|0)|"
   "(?<identifier>\\w(?:\\w|\\d)*)"
   ")"
};

auto const tokenizer_re = ::ctre::tokenize<
   lex_patterns
>;

namespace Tokens {

struct Base {
   ::std::string orig_text_;
};

struct UnsignedInteger : Base {
   ::std::uintmax_t value_;
};

struct Identifier : Base {
   ::std::string value_;
};

using AnyToken = ::std::variant<UnsignedInteger, Identifier>;

}

template <::std::forward_iterator I>
::std::vector<::std::string> tokenize_tokenizer(I begin, I end)
{
   ::std::vector<::std::string> result;
   for (auto token: tokenizer_re(begin, end)) {
      if (token) {
         ::std::clog << "got one\n";
         auto const &full_capture = token.to_string();
         result.emplace_back(full_capture.begin(), full_capture.end());
      }
   }
   return result;
}

void tokenize_test()
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
      ::std::cout << ::std::format(" - [\"{}\"]\n", token);
   }
}

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

int main()
{
   using rawchars_t = ::std::istreambuf_iterator<char>;
   input_to_forward_range_adapter adapter{
      rawchars_t{::std::cin}, rawchars_t{}
   };
   auto matches = tokenizer_re(adapter.begin(), adapter.end());
   ::std::vector<Tokens::AnyToken> tokens;
   using ::std::format;
   using ::std::cout;
   using ::std::stoull;
   for (auto const &match: matches) {
      if (match) {
         ::std::clog << "got one" << ::std::endl;
         ::std::clog << ::std::format(" - [\"{}\"]\n", match.to_string()) << ::std::flush;
         if (auto const &dec = match.get<"int_dec">()) {
            auto const num = dec.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 10)});
         }
         if (auto const &hex = match.get<"int_hex">()) {
            auto const num = hex.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 16)});
         }
         if (auto const &oct = match.get<"int_oct">()) {
            auto const num = oct.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 8)});
         }
         if (auto const &id = match.get<"identifier">()) {
            tokens.emplace_back(Tokens::Identifier{id.to_string(), id.to_string()});
         }
      }
   }
   const auto visitor = overloads
   {
      [](Tokens::UnsignedInteger const &t) {
         ::std::cout << format("num: \"{}\" -> {}\n", t.orig_text_, t.value_);
      },
      [](Tokens::Identifier const &t) {
         ::std::cout << format(" id: \"{}\" -> {}\n", t.orig_text_, t.value_);
      }
   };
   for (auto const &token: tokens) {
      ::std::visit(visitor, token);
   }
}

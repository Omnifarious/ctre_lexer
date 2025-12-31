// Copyright © 2025 Eric Hopper. All rights reserved.
// Created on 2025-06-29.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include "tokens.hpp"
#include "parser.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <memory>
#include <cstdlib>

using Tokens::tokenize_input;
template <class... Ts>
using overloads = Tokens::overloads<Ts...>;

int main()
{
   using namespace ::std::literals::string_view_literals;
   using rawchars_t = ::std::istreambuf_iterator<char>;
   Tokens::toklist_t tokens = tokenize_input(
      rawchars_t{::std::cin},
      rawchars_t{}
   );
   using ::std::format;
   using ::std::cout;
   const auto visitor = overloads
   {
      [](Tokens::UnsignedInteger const &t) {
         ::std::cout << format("num: \"{}\" -> {}\n", t.orig_text_, t.value_);
      },
      [](Tokens::Identifier const &t) {
         ::std::cout << format(" id: \"{}\" -> {}\n", t.orig_text_, t.value_);
      },
      [](Tokens::Punctuator const &t) {
         ::std::cout << format(" pun: \"{}\" -> {}\n", t.orig_text_, t.S_val_str[t.value_]);
      },
      [](Tokens::Operator const &t) {
         ::std::cout << format(" op: \"{}\" -> {}\n", t.orig_text_, t.S_val_str[t.value_]);
      },
      [](Tokens::Paren const &t) {
         ::std::cout << format("par: \"{}\" -> {}\n", t.orig_text_, t.S_val_str[t.value_]);
      },
      [](Tokens::CurlyBracket const &t) {
         using namespace ::std::literals::string_view_literals;
         ::std::cout << format("curly_bracket: \"{}\" -> {}\n", t.orig_text_, t.S_val_str[t.value_]);
      },
      [](Tokens::Keyword const &t) {
         ::std::cout << format("keyword: \"{}\" -> {}\n", t.orig_text_, t.S_val_str[t.value_]);
      },
      [](Tokens::Semicolon const &t) {
         ::std::cout << format("sem: \"{}\"\n", t.orig_text_);
      },
   };
   for (auto const &token: tokens) {
      ::std::visit(visitor, token);
   }
   auto const [expr, remainder] =
      Parser::parse_top(tokens.begin(), tokens.end());
   if (remainder != tokens.end()) {
      ::std::cerr << "Parse error: unexpected token at end of input.\n";
   }
   if (expr) {
      ::std::cout << "Program:\n======\n"
                  << expr->to_prefix_string()
                  << "\n======\n";
      ::std::cout << "    Result:\n" << expr->evaluate() << "\n";
   } else {
      ::std::cerr << "Parse error: no expression parsed.\n";
   }
   ::std::cout << "Done.\n";
   return 0;
}

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
      ::std::istreambuf_iterator<char>{::std::cin},
      ::std::istreambuf_iterator<char>{}
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
      [](Tokens::Operator const &t) {
         using namespace ::std::literals::string_view_literals;
         using sv = ::std::string_view;
         sv const op_name[] = {"plus"sv, "minus"sv, "multiply"sv, "divide"sv};
         ::std::cout << format(" op: \"{}\" -> {}\n", t.orig_text_, op_name[t.value_]);
      },
      [](Tokens::Paren const &t) {
         using namespace ::std::literals::string_view_literals;
         using sv = ::std::string_view;
         sv const paren_name[] = {"open"sv, "close"sv};
         ::std::cout << format("par: \"{}\" -> {}\n", t.orig_text_, paren_name[t.value_]);
      },
      [](Tokens::Semicolon const &t) {
         ::std::cout << format("sem: \"{}\"\n", t.orig_text_);
      },
      [](Tokens::Equal const &t) {
         ::std::cout << format("equ: \"{}\"\n", t.orig_text_);
      }
   };
   for (auto const &token: tokens) {
      ::std::visit(visitor, token);
   }
   auto const [expr, remainder] =
      Parser::parse_statement_list(tokens.begin(), tokens.end());
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

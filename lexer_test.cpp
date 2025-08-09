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
#include <memory>
#include <type_traits>
#include <fmt/format.h>

static constexpr auto lex_patterns = ctll::fixed_string{
   "(?m)\\s*(?:"
   "(?<int_hex>0x[0-9a-fA-F]+)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_dec>(?:[1-9][0-9]*)|0)|"
   "(?<identifier>\\w(?:\\w|\\d)*)|"
   "(?<operator>[+*/]|-)|"
   "(?<paren>[()])"
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

struct Operator : Base {
   enum { Plus, Minus, Multiply, Divide } value_;
};

struct Paren : Base {
   enum { Open, Close } value_;
};

using AnyToken = ::std::variant<UnsignedInteger, Identifier, Operator, Paren>;

}

template <::std::forward_iterator I>
::std::vector<::std::string> tokenize_tokenizer(I begin, I end)
{
   ::std::vector<::std::string> result;
   for (auto token: tokenizer_re(begin, end)) {
      if (token) {
         // ::std::clog << "got one\n";
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
         //::std::clog << "got one\n" << ::std::flush;
         //::std::clog << ::std::format(" - [\"{}\"]\n", match.to_string()) << ::std::flush;
         if (auto const &dec = match.get<"int_dec">()) {
            auto const num = dec.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 10)});
         } else if (auto const &hex = match.get<"int_hex">()) {
            auto const num = hex.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 16)});
         } else if (auto const &oct = match.get<"int_oct">()) {
            auto const num = oct.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 8)});
         } else if (auto const &id = match.get<"identifier">()) {
            tokens.emplace_back(Tokens::Identifier{id.to_string(), id.to_string()});
         } else if (auto const &op = match.get<"operator">()) {
            switch (op.to_string()[0]) {
             case '+':
               tokens.emplace_back(Tokens::Operator{op.to_string(), Tokens::Operator::Plus});
               break;
             case '-':
               tokens.emplace_back(Tokens::Operator{op.to_string(), Tokens::Operator::Minus});
               break;
             case '*':
               tokens.emplace_back(Tokens::Operator{op.to_string(), Tokens::Operator::Multiply});
               break;
             case '/':
               tokens.emplace_back(Tokens::Operator{op.to_string(), Tokens::Operator::Divide});
               break;
             default:
               assert(!"Unexpected operator");
               break;
            }
         } else if (auto const &paren = match.get<"paren">()) {
            switch (paren.to_string()[0]) {
             case '(':
               tokens.emplace_back(Tokens::Paren{paren.to_string(), Tokens::Paren::Open});
               break;
             case ')':
               tokens.emplace_back(Tokens::Paren{paren.to_string(), Tokens::Paren::Close});
               break;
             default:
               assert(!"Unexpected parenthesis");
               break;
            }
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
      }
   };
   for (auto const &token: tokens) {
      ::std::visit(visitor, token);
   }
}


class Expression {
 public:
   virtual ~Expression() = default;

   virtual ::std::uintmax_t evaluate() const = 0;
   virtual ::std::string to_infix_string() const = 0;
   virtual ::std::string to_prefix_string() const = 0;
};

using exprptr_t = ::std::unique_ptr<Expression>;

class BinaryOperation : public Expression {
 public:
   using OpType = decltype(::std::declval<Tokens::Operator>().value_);
   BinaryOperation(OpType op, exprptr_t left, exprptr_t right) :
       op_(op), left_(::std::move(left)), right_(::std::move(right))
   {}

   ::std::uintmax_t evaluate() const override { return 0; }
   ::std::string to_infix_string() const override;
   ::std::string to_prefix_string() const override;
 private:
   OpType op_;
   exprptr_t left_;
   exprptr_t right_;
};

class IdentifierExpression : public Expression {
 public:
   explicit IdentifierExpression(Tokens::Identifier const &idtok) :
       name_(idtok.value_)
   {}

   ::std::uintmax_t evaluate() const override { return 0; }
   ::std::string to_infix_string() const override { return name_; }
   ::std::string to_prefix_string() const override { return name_; }

 private:
   ::std::string name_;
};

class NumericLiteral : public Expression {
 public:
   NumericLiteral(::std::uintmax_t value) : value_(value)
   {}

   ::std::uintmax_t evaluate() const override { return value_; }
   ::std::string to_infix_string() const override { return ::std::to_string(value_); }
   ::std::string to_prefix_string() const override { return ::std::to_string(value_); }

 private:
   ::std::uintmax_t value_;
};


// expression <- term | term ( + | - ) expression
// term <- factor | factor ( * | / ) term | "(" expression ")"
// factor <- identifer | numeric_literal


using toklist_t = ::std::vector<Tokens::AnyToken>;
using parse_result_t = ::std::pair<exprptr_t, toklist_t::iterator>;

parse_result_t
parse_term(toklist_t::iterator start, toklist_t::iterator finish);
parse_result_t
parse_factor(toklist_t::iterator start, toklist_t::iterator finish);

parse_result_t parse_expression(
   ::std::vector<Tokens::AnyToken>::iterator start,
   ::std::vector<Tokens::AnyToken>::iterator finish
   )
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   auto [term, remainder] = parse_term(start, finish);
   if (remainder == finish) {
      return parse_result_t{::std::move(term), remainder};
   } else if (!term) {
      ::std::cerr << "Expected term, parse aborted.\n";
      return parse_result_t{nullptr, finish};
   }
   auto const &op = *remainder;
   if (!::std::holds_alternative<Tokens::Operator>(op)) {
      ::std::cerr << "Expected operator, parse aborted.\n";
      return parse_result_t{nullptr, finish};
   }
   auto const &opval = ::std::get<Tokens::Operator>(op).value_;
   if (opval != Tokens::Operator::Plus && opval != Tokens::Operator::Minus) {
      ::std::cerr << "Expected + or -, parse aborted.\n";
      ::std::cerr << "This is likely a programming error.\n";
      return parse_result_t{nullptr, finish};
   }
   ++remainder;
   auto [rterm, rremainder] = parse_expression(remainder, finish);
   if (rterm) {
      return parse_result_t{
         ::std::make_unique<BinaryOperation>(opval, ::std::move(term), ::std::move(rterm)),
         rremainder
      };
   } else {
      ::std::cerr << "Expected expression, parse aborted.\n";
      return parse_result_t{nullptr, finish};
   }
}

parse_result_t
parse_term(toklist_t::iterator start, toklist_t::iterator finish)
{
   if (start == finish) {
      return {nullptr, finish};
   }
   auto [factor, remainder] = parse_factor(start, finish);
   if (remainder == finish) {
      return parse_result_t{::std::move(factor), remainder};
   } else if (!factor) {
      ::std::cerr << "Expected factor, parse aborted.\n";
      return {nullptr, finish};
   }
   auto const &op = *remainder;
   if (!::std::holds_alternative<Tokens::Operator>(op)) {
      ::std::cerr << "Expected operator, parse aborted.\n";
      return {nullptr, finish};
   }
   auto const &opval = ::std::get<Tokens::Operator>(op).value_;
   if (opval != Tokens::Operator::Multiply && opval != Tokens::Operator::Divide) {
      ::std::cerr << "Expected * or /, parse aborted.\n";
      ::std::cerr << "This is likely a programming error.\n";
      return {nullptr, finish};
   }
   ++remainder;
   auto [rfactor, rremainder] = parse_term(remainder, finish);
   if (rfactor) {
      return {
         ::std::make_unique<BinaryOperation>(opval, ::std::move(factor), ::std::move(rfactor)),
         rremainder
      };
   } else {
      ::std::cerr << "Expected factor, parse aborted.\n";
      return {nullptr, finish};
   }
}

parse_result_t parse_factor(toklist_t::iterator start, toklist_t::iterator finish)
{
   if (start == finish) {
      return {nullptr, finish};
   }
   auto const &tok = *start;
   if (auto const id = ::std::get_if<Tokens::Identifier>(&tok)) {
      return {
         ::std::make_unique<IdentifierExpression>(*id),
         ::std::next(start)
      };
   } else if (auto const num = ::std::get_if<Tokens::UnsignedInteger>(&tok)) {
      return {
         ::std::make_unique<NumericLiteral>(num->value_),
         ::std::next(start)
      };
   }
   ::std::cerr << "Expected factor, parse aborted.\n";
   return {nullptr, finish};
}

// Copyright 2025 by Eric Hopper
// See project LICENSE file for more details.

#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <ctre.hpp>
#include <string>
#include <cstdint>
#include <vector>
#include <variant>
#include <cassert>
#include <iterator>
#include "ring_buffer_view.hpp"

namespace Tokens {

static constexpr auto lex_patterns = ctll::fixed_string{
   "(?m)\\s*(?:"
   "(?<int_hex>0x[0-9a-fA-F]+)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_dec>(?:[1-9][0-9]*)|0)|"
   "(?:(?<keyword>(?:if|else|while))(?!\\w))|"
   "(?<identifier>\\w(?:\\w|\\d)*)|"
   "(?<operator><=|>=|!=|&&|\\|\\||[+*/=<>]|-)|"
   "(?<paren>[()])|"
   "(?<semicolon>;)|"
   "(?<curly_bracket>\\{|\\})|"
   "(?<unknown>\\S+)"
   ")"
};

auto const tokenizer_re = ::ctre::tokenize<
   lex_patterns
>;

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
   enum {
      Plus, Minus, Multiply, Divide, BoolAnd, BoolOr,
      Equal, Greater, Less, GreaterEqual, LessEqual, NotEqual
   } value_;
};

struct Paren : Base {
   enum { Open, Close } value_;
};

struct CurlyBracket : Base {
   enum { Open, Close } value_;
};

struct Keyword : Base {
   enum { If, Else, While } value_;
};

struct Semicolon : Base {};

using AnyToken = ::std::variant<
   UnsignedInteger, Identifier,
   Operator, Paren, Semicolon, CurlyBracket,
   Keyword
>;

using toklist_t = ::std::vector<AnyToken>;

class tokenize_error : public ::std::runtime_error {
public:
   using ::std::runtime_error::runtime_error;
};

template <::std::input_iterator I>
toklist_t tokenize_input(I begin, I end)
{
   using ::std::stoull;
   using ::std::get;
   ::std::vector<AnyToken> tokens;
   input_to_forward_range_adapter adapter{begin, end};
   for (auto const &match: tokenizer_re(adapter.begin(), adapter.end())) {
      if (match) {
         if (auto const &dec = match.template get<"int_dec">()) {
            auto const num = dec.to_string();
            tokens.emplace_back(UnsignedInteger{num, stoull(num, nullptr, 10)});
         } else if (auto const &hex = match.template get<"int_hex">()) {
            auto const num = hex.to_string();
            tokens.emplace_back(UnsignedInteger{num, stoull(num, nullptr, 16)});
         } else if (auto const &oct = match.template get<"int_oct">()) {
            auto const num = oct.to_string();
            tokens.emplace_back(UnsignedInteger{num, stoull(num, nullptr, 8)});
         } else if (auto const &id = match.template get<"identifier">()) {
            tokens.emplace_back(Identifier{id.to_string(), id.to_string()});
         } else if (auto const &op = match.template get<"operator">()) {
            switch (op.to_string()[0]) {
               case '+':
                  tokens.emplace_back(Operator{op.to_string(), Operator::Plus});
                  break;
               case '-':
                  tokens.emplace_back(Operator{op.to_string(), Operator::Minus});
                  break;
               case '*':
                  tokens.emplace_back(Operator{op.to_string(), Operator::Multiply});
                  break;
               case '/':
                  tokens.emplace_back(Operator{op.to_string(), Operator::Divide});
                  break;
               case '=':
                  tokens.emplace_back(Operator{op.to_string(), Operator::Equal});
                  break;
               case '<':
                  if (op.to_string() == "<=") {
                     tokens.emplace_back(Operator{op.to_string(), Operator::LessEqual});
                  } else {
                     tokens.emplace_back(Operator{op.to_string(), Operator::Less});
                  }
                  break;
               case '>':
                  if (op.to_string() == ">=") {
                     tokens.emplace_back(Operator{op.to_string(), Operator::GreaterEqual});
                  } else {
                     tokens.emplace_back(Operator{op.to_string(), Operator::Greater});
                  }
                  break;
               case '!':
                  if (op.to_string() == "!=") {
                     tokens.emplace_back(Operator{op.to_string(), Operator::NotEqual});
                     break;
                  }
                  [[fallthrough]];
               case '&':
                  if (op.to_string() == "&&") {
                     tokens.emplace_back(Operator{op.to_string(), Operator::BoolAnd});
                     break;
                  }
                  [[fallthrough]];
               case '|':
                  if (op.to_string() == "||") {
                     tokens.emplace_back(Operator{op.to_string(), Operator::BoolOr});
                     break;
                  }
                  [[fallthrough]];
               default:
                  assert(!"Unexpected operator");
                  break;
            }
         } else if (auto const &paren = match.template get<"paren">()) {
            switch (paren.to_string()[0]) {
               case '(':
                  tokens.emplace_back(Paren{paren.to_string(), Paren::Open});
                  break;
               case ')':
                  tokens.emplace_back(Paren{paren.to_string(), Paren::Close});
                  break;
               default:
                  assert(!"Unexpected parenthesis");
                  break;
            }
         } else if (auto const &brace = match.template get<"curly_bracket">()) {
            switch (brace.to_string()[0]) {
               case '{':
                  tokens.emplace_back(CurlyBracket{brace.to_string(), CurlyBracket::Open});
                  break;
               case '}':
                  tokens.emplace_back(CurlyBracket{brace.to_string(), CurlyBracket::Close});
                  break;
               default:
                  assert(!"Unexpected brace");
                  break;
            }
         } else if (auto const &keyword = match.template get<"keyword">()) {
            if (keyword.to_string() == "if") {
               tokens.emplace_back(Keyword{keyword.to_string(), Keyword::If});
            } else if (keyword.to_string() == "else") {
               tokens.emplace_back(Keyword{keyword.to_string(), Keyword::Else});
            } else if (keyword.to_string() == "while") {
               tokens.emplace_back(Keyword{keyword.to_string(), Keyword::While});
            } else {
               assert(!"Unexpected keyword");
            }
         } else if (auto const &semicolon = match.template get<"semicolon">()) {
            tokens.emplace_back(Semicolon{semicolon.to_string()});
         } else if (auto const &unknown = match.template get<"unknown">()) {
            throw tokenize_error("Unknown token: " + unknown.to_string());
         } else {
            assert(!"Failed to handle token.");
         }
      } else {
         throw tokenize_error("Unable to tokenize starting at...");
      }
   }
   return tokens;
}

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

} // namespace Tokens

#endif //TOKENS_HPP

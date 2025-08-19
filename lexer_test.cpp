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
#include <cstdlib>
#include <unordered_map>
#include <type_traits>
#include <fmt/format.h>

using namespace ::std::literals::string_view_literals;

static constexpr auto lex_patterns = ctll::fixed_string{
   "(?m)\\s*(?:"
   "(?<int_hex>0x[0-9a-fA-F]+)|"
   "(?<int_oct>0[0-7]*[1-7][0-7]*)|"
   "(?<int_dec>(?:[1-9][0-9]*)|0)|"
   "(?<identifier>\\w(?:\\w|\\d)*)|"
   "(?<operator>[+*/]|-)|"
   "(?<paren>[()])|"
   "(?<semicolon>;)|"
   "(?<equal>=)|"
   "(?<unknown>\\S+)"
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

struct Semicolon : Base {};

struct Equal : Base {};

using AnyToken = ::std::variant<
   UnsignedInteger, Identifier, Operator, Paren, Semicolon, Equal
>;

}

class ParseNode {
public:
   virtual ~ParseNode() = default;

   virtual ::std::uintmax_t evaluate() const = 0;
   virtual ::std::string to_infix_string() const = 0;
   virtual ::std::string to_prefix_string() const = 0;
};

using exprptr_t = ::std::unique_ptr<ParseNode>;
using toklist_t = ::std::vector<Tokens::AnyToken>;
using parse_result_t = ::std::pair<exprptr_t, toklist_t::iterator>;

parse_result_t
parse_statement(toklist_t::iterator start, toklist_t::iterator finish);
parse_result_t
parse_expression(toklist_t::iterator start, toklist_t::iterator finish);
parse_result_t
parse_term(toklist_t::iterator start, toklist_t::iterator finish);
parse_result_t
parse_factor(toklist_t::iterator start, toklist_t::iterator finish);

class parse_error : public ::std::runtime_error {
 public:
   using ::std::runtime_error::runtime_error;
};

template <::std::input_iterator I>
::std::vector<Tokens::AnyToken> tokenize_input(I begin, I end)
{
   using ::std::stoull;
   using ::std::get;
   ::std::vector<Tokens::AnyToken> tokens;
   input_to_forward_range_adapter adapter{begin, end};
   for (auto const &match: tokenizer_re(adapter.begin(), adapter.end())) {
      if (match) {
         //::std::clog << "got one\n" << ::std::flush;
         //::std::clog << ::std::format(" - [\"{}\"]\n", match.to_string()) << ::std::flush;
         if (auto const &dec = match.template get<"int_dec">()) {
            auto const num = dec.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 10)});
         } else if (auto const &hex = match.template get<"int_hex">()) {
            auto const num = hex.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 16)});
         } else if (auto const &oct = match.template get<"int_oct">()) {
            auto const num = oct.to_string();
            tokens.emplace_back(Tokens::UnsignedInteger{num, stoull(num, nullptr, 8)});
         } else if (auto const &id = match.template get<"identifier">()) {
            tokens.emplace_back(Tokens::Identifier{id.to_string(), id.to_string()});
         } else if (auto const &op = match.template get<"operator">()) {
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
         } else if (auto const &paren = match.template get<"paren">()) {
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
         } else if (auto const &semicolon = match.template get<"semicolon">()) {
            tokens.emplace_back(Tokens::Semicolon{semicolon.to_string()});
         } else if (auto const &equal = match.template get<"equal">()) {
            tokens.emplace_back(Tokens::Equal{equal.to_string()});
         } else {
            throw parse_error("Unexpected token: " + match.to_string());
         }
      }
   }
   return tokens;
}

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

int main()
{
   using rawchars_t = ::std::istreambuf_iterator<char>;
   toklist_t tokens = tokenize_input(
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
   auto const [expr, remainder] = parse_statement(tokens.begin(), tokens.end());
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

static ::std::unordered_map<::std::string, ::std::uint64_t> identifiers_S;

class BinaryOperation : public ParseNode {
 public:
   using OpType = decltype(::std::declval<Tokens::Operator>().value_);
   BinaryOperation(OpType op, exprptr_t left, exprptr_t right) :
       op_(op), left_(::std::move(left)), right_(::std::move(right))
   {}

   ::std::uintmax_t evaluate() const override;
   ::std::string to_infix_string() const override;
   ::std::string to_prefix_string() const override;

 private:
   OpType op_;
   exprptr_t left_;
   exprptr_t right_;

   using sv = ::std::string_view;
   static sv constexpr S_op_names[] = {"+"sv, "-"sv, "*"sv, "/"sv};
};

::std::uintmax_t BinaryOperation::evaluate() const
{
   auto const lval = left_->evaluate();
   auto const rval = right_->evaluate();
   using namespace Tokens;

   switch (op_) {
    case Operator::Plus:
      return lval + rval;
      break;
    case Operator::Minus:
      return lval - rval;
      break;
    case Operator::Multiply:
      return lval * rval;
      break;
    case Operator::Divide:
      return lval / rval;
      break;
    default:
      assert(!"Unexpected operator");
   }
   return 0;
}

// The problem that needs to be solved here...  how to deal with precedence
// inversion. The simplest option is to just parenthesize everything.
//
//        *
//    ,--/ \--.
//    +       *
//  ,/ \.   ,/ \.
//  1   2   3   4
//
//  ((1 + 2) * (3 * 4))

::std::string BinaryOperation::to_infix_string() const
{
   return ::std::format("({} {} {})", left_->to_infix_string(), S_op_names[op_], right_->to_infix_string());
}

::std::string BinaryOperation::to_prefix_string() const
{
   return ::std::format("({} {} {})", S_op_names[op_], left_->to_prefix_string(), right_->to_prefix_string());
}

class IdentifierExpression : public ParseNode {
 public:
   explicit IdentifierExpression(Tokens::Identifier const &idtok) :
       name_(idtok.value_)
   {}

   ::std::uintmax_t evaluate() const override;
   ::std::string to_infix_string() const override { return name_; }
   ::std::string to_prefix_string() const override { return name_; }

 private:
   ::std::string name_;
};

::std::uintmax_t IdentifierExpression::evaluate() const
{
   auto const it = identifiers_S.find(name_);
   if (it == identifiers_S.end()) {
      return 0;
   }
   return it->second;
}

class NumericLiteral : public ParseNode {
 public:
   explicit NumericLiteral(::std::uintmax_t value) : value_(value)
   { }

   ::std::uintmax_t evaluate() const override { return value_; }
   ::std::string to_infix_string() const override { return ::std::to_string(value_); }
   ::std::string to_prefix_string() const override { return ::std::to_string(value_); }

 private:
   ::std::uintmax_t const value_;
};

parse_result_t parse_statement(
   toklist_t::iterator start,
   toklist_t::iterator finish
   );

class StatementList : public ParseNode {
 public:
   StatementList() = default;

   ::std::uintmax_t evaluate() const override;
   ::std::string to_infix_string() const override;
   ::std::string to_prefix_string() const override;

 private:
   friend parse_result_t parse_statement(
      toklist_t::iterator start,
      toklist_t::iterator finish
   );

   ::std::vector<exprptr_t> statements_;
};

::std::uintmax_t StatementList::evaluate() const
{
   int i = 0;
   for (auto const &statement: statements_) {
      ::std::cout << ::std::format("Statement {}: {}\n", i++, statement->evaluate());
   }
   return 0;
}

::std::string StatementList::to_infix_string() const
{
   ::std::string result;
   for (auto const &statement: statements_) {
      result += statement->to_infix_string();
      result += ";\n";
   }
   return result;
}

::std::string StatementList::to_prefix_string() const
{
   ::std::string result = "(progn\n";
   for (auto const &statement: statements_) {
      result += "    ";
      result += statement->to_prefix_string();
      result += "\n";
   }
   result += ")";
   return result;
}

class AssignmentStatement : public ParseNode {
 public:
   AssignmentStatement(
      Tokens::Identifier const &id, exprptr_t expression
   )
         : id_(id.value_), expression_(::std::move(expression))
   {}

   ::std::uintmax_t evaluate() const override;
   ::std::string to_infix_string() const override;
   ::std::string to_prefix_string() const override;

 private:
   ::std::string id_;
   exprptr_t expression_;
};

::std::uintmax_t AssignmentStatement::evaluate() const
{
   auto const value = expression_->evaluate();
   identifiers_S[id_] = value;
   return 0;
}

::std::string AssignmentStatement::to_infix_string() const
{
   return ::std::format("{} = {};", id_, expression_->to_infix_string());
}

::std::string AssignmentStatement::to_prefix_string() const
{
   return ::std::format("(setq {} {})", id_, expression_->to_prefix_string());
}

// Here is a sort of pseudo-BNF for what's being parsed.
//
// sequence = statement ; [ sequence ]
// statement = expression | identifer = expression
// expression <- term | term ( + | - ) expression
// term <- factor | factor ( * | / ) term
// factor <- identifer | numeric_literal | "(" expression ")"

parse_result_t parse_statement(
   toklist_t::iterator start,
   toklist_t::iterator finish
   )
{
   auto statements = ::std::make_unique<StatementList>();

   while (start != finish) {
      exprptr_t statement;
      toklist_t::iterator after;
      if (auto [expr, remainder] = parse_expression(start, finish); expr) {
         statement = ::std::move(expr);
         after = remainder;
      } else if (::std::holds_alternative<Tokens::Identifier>(*start)) {
         auto const &idtok = ::std::get<Tokens::Identifier>(*start);
         ++start;
         if (start != finish) {
            if (!::std::holds_alternative<Tokens::Equal>(*start)) {
               ::std::cerr << "Expected =, aborting.\n";
               return parse_result_t{nullptr, finish};
            }
            ++start;
            auto [expr, remainder] = parse_expression(start, finish);
            if (!expr) {
               ::std::cerr << "Didn't find expression after = in assignment, aborting!\n";
               return parse_result_t{nullptr, finish};
            }
            statement = ::std::make_unique<AssignmentStatement>(idtok, ::std::move(expr));
            after = remainder;
         }
      } else {
         ::std::cerr << "Not expression or assignment statement, aborting.";
         return parse_result_t{nullptr, finish};
      }
      if (!::std::holds_alternative<Tokens::Semicolon>(*after)) {
         ::std::cerr << "Expected semicolon, aborting.\n";
         return parse_result_t{nullptr, finish};
      }
      start = ++after;
      statements->statements_.emplace_back(::std::move(statement));
   }
   return parse_result_t{::std::move(statements), finish};
}

parse_result_t parse_expression(
   toklist_t::iterator start,
   toklist_t::iterator finish
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
   if (::std::holds_alternative<Tokens::Equal>(op)) {
      return parse_result_t{nullptr, finish};
   }
   if (!::std::holds_alternative<Tokens::Operator>(op)) {
      return parse_result_t{::std::move(term), remainder};
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
      return parse_result_t{::std::move(factor), remainder};
   }
   auto const &opval = ::std::get<Tokens::Operator>(op).value_;
   if (opval != Tokens::Operator::Multiply && opval != Tokens::Operator::Divide) {
      return parse_result_t{::std::move(factor), remainder};
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
   } else if (auto const op = ::std::get_if<Tokens::Paren>(&tok)) {
      if (op->value_ == Tokens::Paren::Open) {
         ++start;
         auto [expr, remainder] = parse_expression(start, finish);
         if (remainder == finish) {
            ::std::cerr << "Expected expression, parse aborted.\n";
            return {nullptr, finish};
         }
         if (auto const opclose = ::std::get_if<Tokens::Paren>(&(*remainder))) {
            if (opclose->value_ == Tokens::Paren::Close) {
               return {::std::move(expr), ::std::next(remainder)};
            }
            ::std::cerr << "Expected closing paren, parse aborted.\n";
            return {nullptr, finish};
         } else {
            ::std::cerr << "Expected closing paren, parse aborted.\n";
            return {nullptr, finish};
         }
      }
   }
   ::std::cerr << "Expected factor, parse aborted.\n";
   return {nullptr, finish};
}

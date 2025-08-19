// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include <unordered_map>
#include "parser.hpp"
#include "tokens.hpp"

static ::std::unordered_map<::std::string, ::std::uint64_t> identifiers_S;
using namespace ::std::string_view_literals;

namespace Parser {
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
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
   );

class StatementList : public ParseNode {
public:
   StatementList() = default;

   ::std::uintmax_t evaluate() const override;
   ::std::string to_infix_string() const override;
   ::std::string to_prefix_string() const override;

private:
   friend parse_result_t parse_statement(
      Tokens::toklist_t::iterator start,
      Tokens::toklist_t::iterator finish
   );

   ::std::vector<exprptr_t> statements_;
};

::std::uintmax_t StatementList::evaluate() const
{
   int i = 0;
   decltype(statements_[0]->evaluate()) result = 0U;
   for (auto const &statement: statements_) {
      result = statement->evaluate();
      ::std::cout << ::std::format("Statement {}: {}\n", i++, result);
   }
   return result;
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
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
   )
{
   auto statements = ::std::make_unique<StatementList>();

   while (start != finish) {
      exprptr_t statement;
      Tokens::toklist_t::iterator after;
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
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
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
parse_term(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
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

parse_result_t parse_factor(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
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

} // namespace Parser
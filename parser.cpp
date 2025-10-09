// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include <deque>
#include <sstream>
#include <array>
#include "parser.hpp"
#include "tokens.hpp"

using namespace ::std::string_view_literals;

namespace {
using sv = ::std::string_view;
constexpr auto S_op_names = ::std::array{
   "+"sv, "-"sv, "*"sv, "/"sv, "&&"sv, "||"sv,
   "="sv, ">"sv, "<"sv, ">="sv, "<="sv, "!="sv
   };
}

namespace Parser {
void SimpleEvaluator::operator()(BinaryOperation const &op)
{
   ::std::visit(*this, *op.left_);
   auto lval = current_result_;
   ::std::visit(*this, *op.right_);
   auto rval = current_result_;
   using namespace Tokens;

   switch (op.op_) {
      case Operator::Plus:
         current_result_ = lval + rval;
         break;
      case Operator::Minus:
         current_result_ = lval - rval;
         break;
      case Operator::Multiply:
         current_result_ = lval * rval;
         break;
      case Operator::Divide:
         current_result_ = lval / rval;
         break;
      case Operator::BoolAnd:
         current_result_ = lval && rval ? 1U : 0U;
         break;
      case Operator::BoolOr:
         current_result_ = lval || rval ? 1U : 0U;
         break;
      case Operator::Equal:
         current_result_ = lval == rval ? 1U : 0U;
         break;
      case Operator::NotEqual:
         current_result_ = lval != rval ? 1U : 0U;
         break;
      case Operator::Less:
         current_result_ = lval < rval ? 1U : 0U;
         break;
      case Operator::LessEqual:
         current_result_ = lval <= rval ? 1U : 0U;
         break;
      case Operator::Greater:
         current_result_ = lval > rval ? 1U : 0U;
         break;
      case Operator::GreaterEqual:
         current_result_ = lval >= rval ? 1U : 0U;
         break;
      default:
         assert(!"Unexpected operator");
         current_result_ = 0U;
   }
}

void SimpleEvaluator::operator()(Identifier const &id)
{
   auto const it = identifiers_.find(id.name_);
   if (it == identifiers_.end()) {
      current_result_ = 0;
   } else {
      current_result_ = it->second;
   }
}

void SimpleEvaluator::operator()(NumericLiteral const &nl)
{
   current_result_ = nl.value_;
}

void SimpleEvaluator::operator()(StatementList const &sl)
{
   for (auto const &statement: sl.statements_) {
      ::std::visit(*this, *statement);
      if (statement_function_) {
         statement_function_(current_result_);
      }
   }
}

void SimpleEvaluator::operator()(AssignmentStatement const &as)
{
   ::std::visit(*this, *as.expression_);
   identifiers_[as.id_] = current_result_;
   current_result_ = 0;
}

void SimpleEvaluator::operator()(IfStatement const &ifs)
{
   ::std::visit(*this, *ifs.condition_);
   auto const cond = current_result_;
   if (cond) {
      ::std::visit(*this, *ifs.then_statement_);
   } else if (ifs.else_statement_) {
      ::std::visit(*this, *ifs.else_statement_);
   }
}

struct InfixStringizer {
   ::std::ostringstream result_;

   void operator()(BinaryOperation const &op)
   {
      result_ << '(';
      ::std::visit(*this, *op.left_);
      assert(op.op_ < S_op_names.size());
      result_ << ' ' << S_op_names[op.op_] << ' ';
      ::std::visit(*this, *op.right_);
      result_ << ')';
   }
   void operator()(Identifier const &id)
   {
      result_ << id.name_;
   }
   void operator()(NumericLiteral const &nl)
   {
      result_ << nl.value_;
   }
   void operator()(StatementList const &sl)
   {
      for (auto const &statement: sl.statements_) {
         ::std::visit(*this, *statement);
         if (!::std::holds_alternative<IfStatement>(*statement))
            result_ << ";\n";
      }
   }
   void operator()(AssignmentStatement const &as)
   {
      result_ << as.id_ << " = ";
      ::std::visit(*this, *as.expression_);
   }
   void operator()(IfStatement const &ifs)
   {
      result_ << "if (";
      ::std::visit(*this, *ifs.condition_);
      result_ << ") {\n";
      ::std::visit(*this, *ifs.then_statement_);
      if (!ifs.else_statement_) {
         result_ << ";\n}\n";
      } else {
         result_ << ";\n} else {\n";
         ::std::visit(*this, *ifs.else_statement_);
         result_ << ";\n}\n";
      }
   }
};

struct PrefixStringizer {
   ::std::ostringstream result_;
   void operator()(BinaryOperation const &op)
   {
      result_ << '(';
      result_ << S_op_names[op.op_] << ' ';
      ::std::visit(*this, *op.left_);
      result_ << ' ';
      ::std::visit(*this, *op.right_);
      result_ << ')';
   }
   void operator()(Identifier const &id)
   {
      result_ << id.name_;
   }
   void operator()(NumericLiteral const &nl)
   {
      result_ << nl.value_;
   }
   void operator()(StatementList const &sl)
   {
      result_ << "(progn\n";
      for (auto const &statement: sl.statements_) {
         result_ << "    ";
         ::std::visit(*this, *statement);
         result_ << "\n";
      }
      result_ << ")";
   }
   void operator()(AssignmentStatement const &as)
   {
      result_ << "(setq " << as.id_ << " ";
      ::std::visit(*this, *as.expression_);
      result_ << ")";
   }
   void operator()(IfStatement const &ifs)
   {
      result_ << "(if (";
      ::std::visit(*this, *ifs.condition_);
      result_ << ") (progn\n";
      ::std::visit(*this, *ifs.then_statement_);
      result_ << ")\n";
      if (!ifs.else_statement_) {
         result_ << ")\n";
      } else {
         result_ << "    (progn\n";
         ::std::visit(*this, *ifs.else_statement_);
         result_ << "))\n";
      }
   }
};

::std::uintmax_t ASTNode::evaluate() const
{
   SimpleEvaluator eval;
   ::std::visit(eval, *this);
   return eval.current_result_;
}

::std::string ASTNode::to_infix_string() const
{
   InfixStringizer is;
   ::std::visit(is, *this);
   return is.result_.str();
}

::std::string ASTNode::to_prefix_string() const
{
   PrefixStringizer ps;
   ::std::visit(ps, *this);
   return ps.result_.str();
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


parse_result_t parse_sequence(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
   )
{
   auto statements_node = ::std::make_unique<ASTNode>(StatementList{});
   {
      auto &statements = ::std::get<StatementList>(*statements_node);

      while (start != finish) {
         auto [statement, remainder] = parse_statement(start, finish);

         if (!statement) {
            break;
         }
         start = remainder;
         statements.statements_.emplace_back(::std::move(statement));
      }
   }
   return parse_result_t{::std::move(statements_node), start};
}

parse_result_t parse_statement(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }

   astptr_t statement;
   Tokens::toklist_t::iterator after = finish;

   // Try each parser in order
   if (
      auto *tok = ::std::get_if<Tokens::CurlyBracket>(&(*start));
      tok && tok->value_ == Tokens::CurlyBracket::Open
   ) {
      start = ::std::next(start);
      auto [stmt, remainder] = parse_sequence(start, finish);
      if (!stmt) {
         ::std::cerr << "Didn't find proper statement sequence after {";
         return parse_result_t{nullptr, finish};
      }
      start = remainder;
      if (
         auto *tok = ::std::get_if<Tokens::CurlyBracket>(&(*start));
         !(tok && tok->value_ == Tokens::CurlyBracket::Close)
      ) {
         ::std::cerr << "Didn't find } after statement sequence, aborting!\n";
         return parse_result_t{nullptr, finish};
      }
      start = ::std::next(start); // Eat the close }
      statement = ::std::move(stmt);
      after = start;
   } else if (
      auto [stmt, remainder] = parse_ifelse(start, finish);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_assignment(start, finish);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_expression_statement(start, finish);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   }
   return parse_result_t{::std::move(statement), after};
}

parse_result_t parse_assignment(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
)
{
   if (auto const id = ::std::get_if<Tokens::Identifier>(&(*start))) {
      auto const &idtok = *id;
      auto const after_id = ::std::next(start);
      if (after_id != finish) {
         if (
            ::std::holds_alternative<Tokens::Operator>(*after_id) &&
            ::std::get<Tokens::Operator>(*after_id).value_ == Tokens::Operator::Equal
         ) {
            auto const after_eq = ::std::next(after_id);
            if (after_eq != finish) {
               auto [expr, remainder] = parse_expression(after_eq, finish);
               if (expr) {
                  if (
                     remainder != finish &&
                     ::std::holds_alternative<Tokens::Semicolon>(*remainder)
                  ) {
                     return {
                        ::std::make_unique<ASTNode>(
                           AssignmentStatement{idtok, ::std::move(expr)}
                        ),
                        ::std::next(remainder)
                     };
                  } else {
                     ::std::cerr << "Didn't find semicolon after assignment, "
                                    "aborting!\n";
                  }
               } else {
                  ::std::cerr << "Didn't find expression after = in "
                                  "assignment, aborting!\n";
               }
            }
         }
      }
   }
   return parse_result_t{nullptr, finish};
}

parse_result_t parse_ifelse(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *kw = ::std::get_if<Tokens::Keyword>(&(*start));
      !(kw && kw->value_ == Tokens::Keyword::If)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      ::std::cerr << "Expected expression after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Open)
   ) {
      ::std::cerr << "Expected ( after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      ::std::cerr << "Expected expression after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   auto [expr, after_exp] = parse_expression(start, finish);
   if (!expr) {
      ::std::cerr << "Expected expression after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   start = after_exp;

   if (start == finish) {
      ::std::cerr << "Expected ) after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Close)
   ) {
      ::std::cerr << "Expected ) after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      ::std::cerr << "Expected then block after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   auto [then_block, after_then] = parse_statement(start, finish);
   if (!then_block) {
      ::std::cerr << "Expected then block after if, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   start = after_then;

   if (start == finish) {
      return parse_result_t{
         ::std::make_unique<ASTNode>(IfStatement{
            ::std::move(expr),
            ::std::move(then_block),
            nullptr
         }),
         start
      };
   }
   if (
      auto const *kw = ::std::get_if<Tokens::Keyword>(&(*start));
      !(kw && kw->value_ == Tokens::Keyword::Else)
   ) {
      return parse_result_t{
         ::std::make_unique<ASTNode>(IfStatement{
            ::std::move(expr),
            ::std::move(then_block),
            nullptr
         }),
         start
      };
   }
   start = ::std::next(start);

   if (start == finish) {
      ::std::cerr << "Expected expression statement after else, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   auto [else_block, after_else] = parse_statement(start, finish);
   if (!else_block) {
      ::std::cerr << "Expected expression statement after else, aborting!\n";
      return parse_result_t{nullptr, finish};
   }

   return parse_result_t{
      ::std::make_unique<ASTNode>(IfStatement{
         ::std::move(expr),
         ::std::move(then_block),
         ::std::move(else_block)
      }),
      after_else
   };
}

parse_result_t parse_expression_statement(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
)
{
   auto [expr, remainder] = parse_expression(start, finish);
   if (expr) {
      if (
         remainder != finish &&
         ::std::holds_alternative<Tokens::Semicolon>(*remainder)
      ) {
         return {::std::move(expr), ::std::next(remainder)};
      }
   }
   return parse_result_t{nullptr, finish};
}


parse_result_t parse_expression(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
   )
{
   ::std::deque<astptr_t> boolterms;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (boolterms.size() == operators.size()) {
      auto [boolterm, remainder] = parse_boolterm(start, finish);
      if (!boolterm) {
         return parse_result_t{nullptr, finish};
      }
      boolterms.emplace_back(::std::move(boolterm));
      start = remainder;
      if (auto const &op = ::std::get_if<Tokens::Operator>(&(*remainder))) {
         auto const opval = op->value_;
         if (
            opval == Tokens::Operator::BoolAnd ||
            opval == Tokens::Operator::BoolOr
         ) {
            operators.emplace_back(*op);
            start = ::std::next(remainder);
         }
      }
   }
   assert(operators.size() + 1 == boolterms.size());
   astptr_t top = ::std::move(boolterms.front());
   boolterms.pop_front();
   while (!operators.empty()) {
      auto const op = operators.front();
      operators.pop_front();
      top = ::std::make_unique<ASTNode>(BinaryOperation{
            op.value_, ::std::move(top), ::std::move(boolterms.front())
      });
      boolterms.pop_front();
   }
   return parse_result_t{::std::move(top), start};
}

parse_result_t parse_boolterm(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
   )
{
   ::std::deque<astptr_t> boolterms;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (true) {
      auto [relclause, remainder] = parse_relclause(start, finish);
      if (!relclause) {
         return parse_result_t{nullptr, finish};
      }
      boolterms.emplace_back(::std::move(relclause));
      start = remainder;
      if (auto const &op = ::std::get_if<Tokens::Operator>(&(*remainder))) {
         auto const opval = op->value_;
         if (
            opval == Tokens::Operator::Less ||
            opval == Tokens::Operator::LessEqual ||
            opval == Tokens::Operator::Greater ||
            opval == Tokens::Operator::GreaterEqual ||
            opval == Tokens::Operator::Equal ||
            opval == Tokens::Operator::NotEqual
         ) {
            operators.emplace_back(*op);
            start = ::std::next(remainder);
         } else {
            break;
         }
      } else {
         break;
      }
   }
   assert(operators.size() + 1 == boolterms.size());
   astptr_t top = ::std::move(boolterms.front());
   boolterms.pop_front();
   while (!operators.empty()) {
      auto const op = operators.front();
      operators.pop_front();
      top = ::std::make_unique<ASTNode>(BinaryOperation{
         op.value_, ::std::move(top), ::std::move(boolterms.front())
      });
      boolterms.pop_front();
   }
   return parse_result_t{::std::move(top), start};
}

parse_result_t parse_relclause(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
)
{
   ::std::deque<astptr_t> relclauses;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (true) {
      auto [term, remainder] = parse_term(start, finish);
      if (!term) {
         return parse_result_t{nullptr, finish};
      }
      relclauses.emplace_back(::std::move(term));
      start = remainder;
      if (auto const &op = ::std::get_if<Tokens::Operator>(&(*remainder))) {
         auto const opval = op->value_;
         if (
            opval == Tokens::Operator::Plus ||
            opval == Tokens::Operator::Minus
         ) {
            operators.emplace_back(*op);
            start = ::std::next(remainder);
         } else {
            break;
         }
      } else {
         break;
      }
   }
   assert(operators.size() + 1 == relclauses.size());
   astptr_t top = ::std::move(relclauses.front());
   relclauses.pop_front();
   while (!operators.empty()) {
      auto const op = operators.front();
      operators.pop_front();
      top = ::std::make_unique<ASTNode>(BinaryOperation{
         op.value_, ::std::move(top), ::std::move(relclauses.front())
      });
      relclauses.pop_front();
   }
   return parse_result_t{::std::move(top), start};
}

parse_result_t
parse_term(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
{
   ::std::deque<astptr_t> factors;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return {nullptr, finish};
   }
   while (true) {
      auto [factor, remainder] = parse_factor(start, finish);
      if (!factor) {
         return {nullptr, finish};
      }
      factors.emplace_back(::std::move(factor));
      start = remainder;

      // Check remainder of production, do we have a / or * followed by
      // another term?
      if (start == finish) {
         break;
      }
      if (auto const op = ::std::get_if<Tokens::Operator>(&(*remainder))) {
         if (
            op->value_ == Tokens::Operator::Multiply ||
            op->value_ == Tokens::Operator::Divide
         ) {
            operators.emplace_back(*op);
            start = ::std::next(remainder);
         } else {
            break;
         }
      } else {
         break;
      }
   }
   assert(operators.size() + 1 == factors.size());
   astptr_t top = ::std::move(factors.front());
   factors.pop_front();
   while (!operators.empty()) {
      auto const op = operators.front();
      operators.pop_front();
      top = ::std::make_unique<ASTNode>(BinaryOperation{
         op.value_, ::std::move(top), ::std::move(factors.front())
      });
      factors.pop_front();
   }
   return {::std::move(top), start};
}

parse_result_t parse_factor(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
{
   if (start == finish) {
      return {nullptr, finish};
   }
   auto const &tok = *start;
   if (auto const id = ::std::get_if<Tokens::Identifier>(&tok)) {
      return {
         ::std::make_unique<ASTNode>(Identifier{*id}),
         ::std::next(start)
      };
   } else if (auto const num = ::std::get_if<Tokens::UnsignedInteger>(&tok)) {
      return {
         ::std::make_unique<ASTNode>(NumericLiteral{num->value_}),
         ::std::next(start)
      };
   } else if (auto const op = ::std::get_if<Tokens::Paren>(&tok)) {
      if (op->value_ == Tokens::Paren::Open) {
         ++start;
         auto [expr, remainder] = parse_boolterm(start, finish);
         if (remainder != finish) {
            if (
               auto const opclose = ::std::get_if<Tokens::Paren>(&(*remainder))
            ) {
               if (opclose->value_ == Tokens::Paren::Close) {
                  return {::std::move(expr), ::std::next(remainder)};
               }
            }
         }
      }
      ::std::cerr << "Expected closing paren, parse aborted.\n";
      return {nullptr, finish};
   }
   return {nullptr, finish};
}

} // namespace Parser
//
// Created by hopper on 8/18/25.
//

#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <cstdint>
#include <string>
#include <utility>
#include <functional>
#include <unordered_map>
#include "tokens.hpp"

namespace Parser {

// Here is a sort of pseudo-BNF for what's being parsed.
//
// sequence = statement [ sequence ]
// statement = assignment | expression_statement
// assignment = identifier "=" expression ";"
// expression_statement = expression ";"
// expression <- expression | expression ( "&&" | "||" ) boolterm
// boolterm = term | boolterm ( "+" | "-" ) term
// term <- factor | term ( "*" | "/" ) factor
// factor <- IDENTIFIER | NUMERIC_LITERAL | "(" expression ")"
//
// This is the current target grammar for the next sequence of changes. But I'm
// going to implement the changes for the above grammar first.
//
// block = "{" [ sequence ] "}"
// sequence = statement [ sequence ]
// statement = if_statement | assignment | expression_statement
// if_statement = IF "(" expression ")" ( block | expression_statement ) [ ELSE ( block | expression_statement ) ]
// assignment = identifier "=" expression ";"
// expression_statement = expression ";"
// expression <- expression | expression ( "&&" | "||" ) boolterm
// boolterm = term | boolterm ( "+" | "-" ) term
// term <- factor | term ( "*" | "/" ) factor
// factor <- IDENTIFIER | NUMERIC_LITERAL | "(" expression ")"

class ASTNode;
using astptr_t = ::std::unique_ptr<ASTNode>;

using parse_result_t = ::std::pair<astptr_t, Tokens::toklist_t::iterator>;

parse_result_t
parse_sequence(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
constexpr auto parse_top = parse_sequence;
parse_result_t
parse_expression_statement(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_assignment(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_expression(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_boolterm(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_term(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_factor(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);

class BinaryOperation {
public:
   using OpType = decltype(::std::declval<Tokens::Operator>().value_);
   // Can't define this here because we don't know what an ASTNode is yet.
   inline BinaryOperation(OpType op, astptr_t left, astptr_t right);

   OpType op_;
   astptr_t left_;
   astptr_t right_;
};

class Identifier {
public:
   explicit Identifier(Tokens::Identifier const &idtok) :
       name_(idtok.value_)
   {}

   ::std::string name_;
};

class NumericLiteral {
public:
   explicit NumericLiteral(::std::uintmax_t value) : value_(value)
   {}

   ::std::uintmax_t const value_;
};

class StatementList {
public:
   StatementList() = default;

   ::std::vector<astptr_t> statements_;
};

class AssignmentStatement {
public:
   // Can't define this here because we don't know what an ASTNode is yet.
   inline AssignmentStatement(
      Tokens::Identifier const &id, astptr_t expression
   );

   ::std::string id_;
   astptr_t expression_;
};

using allnodes_t = ::std::variant<
   BinaryOperation, Identifier, NumericLiteral,
   StatementList, AssignmentStatement
>;

class ASTNode : public allnodes_t {
public:
   using allnodes_t::allnodes_t;

   ::std::uintmax_t evaluate() const;
   ::std::string to_infix_string() const;
   ::std::string to_prefix_string() const;
};

// Now we know what an ASTNode is, so we can define these.
inline BinaryOperation::BinaryOperation(OpType op, astptr_t left, astptr_t right)
   : op_(op), left_(::std::move(left)), right_(::std::move(right))
{
}

inline AssignmentStatement::AssignmentStatement(Tokens::Identifier const &id, astptr_t expression)
   : id_(id.value_), expression_(::std::move(expression))
{
}

class SimpleEvaluator {
 public:
   SimpleEvaluator() = default;
   explicit SimpleEvaluator(::std::function<void(uintmax_t statement_result)> f)
      : statement_function_(::std::move(f))
   {}
   void operator()(BinaryOperation const &op);
   void operator()(Identifier const &id);
   void operator()(NumericLiteral const &lit);
   void operator()(StatementList const &list);
   void operator()(AssignmentStatement const &as);
   void operator()(ASTNode const &node);

   ::std::uintmax_t current_result_ = 0;
   ::std::function<void(uintmax_t statement_result)> statement_function_;
   ::std::unordered_map<std::string, ::std::uintmax_t> identifiers_;
};

} // namespace Parser

#endif //PARSER_HPP

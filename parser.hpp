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
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "tokens.hpp"

namespace Parser {

// Here is a sort of pseudo-BNF for what's being parsed.
//
// sequence = statement [ sequence ]
// statement = if_statement | while_statement | assignment |
//             expression_statement | "{" sequence "}"
// var_declaration = VAR IDENTIFIER "=" expression ";"
// func_declaration = DEF IDENTIFIER "(" [ identifier_list ] ")" "{" sequence "}"
// if_statement = IF "(" expression ")" statement [ ELSE statement ]
// while_statement = WHILE "(" expression ")" statement
// assignment = identifier "=" expression ";"
// expression_statement = expression ";"
// expr_list = expression | expr_list "," expression
// identifier_list = IDENTIFIER | identifier_list "," IDENTIFIER
// expression = expression | expression ( "&&" | "||" ) boolterm
// boolterm = relclause | boolterm ( "<" | ">" | "<=" | ">=" | "=" | "!=" ) relclause
// relclause = term | relclause ( "+" | "-" ) term
// term = factor | term ( "*" | "/" ) factor
// function_call = IDENTIFIER "(" [ expr_list ] ")"
// factor = function_call | IDENTIFIER | NUMERIC_LITERAL | "(" expression ")"

class ASTNode;
using astptr_t = ::std::unique_ptr<ASTNode>;

using parse_result_t = ::std::pair<astptr_t, Tokens::toklist_t::iterator>;

namespace priv_ {
class parse_context;
}

parse_result_t
parse_top(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish
);

parse_result_t parse_new_scope(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &ctx
);

parse_result_t
parse_sequence(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context,
   astptr_t slist_node
);

parse_result_t
parse_statement(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_expression_statement(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_assignment(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_vardecl(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_func_declaration(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_func_call(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_ifelse(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_while(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_expression(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_boolterm(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_relclause(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_term(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);
parse_result_t
parse_factor(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);

parse_result_t
parse_identifier(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   priv_::parse_context &context
);

class BinaryOperation {
public:
   using OpType = decltype(::std::declval<Tokens::Operator>().value_);
   // Can't define this here because we don't know what an ASTNode is yet.
   inline BinaryOperation(OpType op, astptr_t left, astptr_t right);

   OpType op_;
   astptr_t left_;
   astptr_t right_;
};

class NumericLiteral {
public:
   explicit NumericLiteral(::std::uintmax_t value) : value_(value)
   {}

   ::std::uintmax_t const value_;
};

struct VarInfo {
   enum vartype_t { UInt64, Function };

   VarInfo(::std::string const &name, vartype_t type)
      : name_(name), type_(type)
   {}
   ::std::string name_;
   vartype_t type_;
};

class StatementList {
public:
   StatementList() = default;

   ::std::vector<astptr_t> statements_;
   ::std::vector<VarInfo> var_declarations_;

   using varidx_t = decltype(var_declarations_)::size_type;
};

class Identifier {
public:
   explicit Identifier(StatementList *scope, StatementList::varidx_t varidx)
        : scope_(scope), varidx_(varidx)
   {}

   StatementList *scope_;
   StatementList::varidx_t varidx_;
};

class FuncDeclaration {
public:
   Identifier name_;
   astptr_t body_;
   StatementList::varidx_t num_args_;
};

class FuncCall {
public:
   Identifier func_;
   ::std::vector<astptr_t> param_exprs_;
};

class AssignmentStatement {
public:
   // Can't define this here because we don't know what an ASTNode is yet.
   inline AssignmentStatement(Identifier id, astptr_t expression);

   Identifier identifier_;
   astptr_t expression_;
};

class VarDecl {
public:
   // Can't define this here because we don't know what an ASTNode is yet.
   inline VarDecl(Identifier id, astptr_t expression);

   Identifier identifier_;
   astptr_t expression_;
};

class IfStatement {
 public:
   inline IfStatement(
      astptr_t condition, astptr_t then_statement, astptr_t else_statement
   );
   inline IfStatement(astptr_t condition, astptr_t then_statement);

   astptr_t condition_;
   astptr_t then_statement_;
   astptr_t else_statement_;
};

class WhileStatement {
 public:
   inline WhileStatement(astptr_t condition, astptr_t repeated);

   astptr_t condition_;
   astptr_t repeated_;
};

using allnodes_t = ::std::variant<
   BinaryOperation, Identifier, NumericLiteral,
   StatementList, AssignmentStatement, IfStatement, WhileStatement,
   VarDecl, FuncDeclaration, FuncCall
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

inline VarDecl::VarDecl(Identifier id, astptr_t expression)
   : identifier_(::std::move(id)), expression_(::std::move(expression))
{
}

inline AssignmentStatement::AssignmentStatement(Identifier id, astptr_t expression)
   : identifier_(::std::move(id)), expression_(::std::move(expression))
{
}

inline IfStatement::IfStatement(
   astptr_t condition, astptr_t then_statement, astptr_t else_statement
)
   : condition_(::std::move(condition)),
     then_statement_(::std::move(then_statement)),
     else_statement_(::std::move(else_statement))
{
}

inline IfStatement::IfStatement(astptr_t condition, astptr_t then_statement)
   : IfStatement(::std::move(condition), ::std::move(then_statement), nullptr)
{
}

inline WhileStatement::WhileStatement(astptr_t condition, astptr_t repeated)
   : condition_(::std::move(condition)), repeated_(::std::move(repeated))
{
}

class SimpleEvaluator {
 public:
   SimpleEvaluator() = default;
   explicit SimpleEvaluator(::std::function<void(uintmax_t statement_result)> f)
      : statement_function_(::std::move(f))
   { }
   void operator()(BinaryOperation const &op);
   void operator()(Identifier const &id);
   void operator()(NumericLiteral const &lit);
   void operator()(StatementList const &list);
   void operator()(AssignmentStatement const &as);
   void operator()(IfStatement const &ifstmt);
   void operator()(WhileStatement const &whilestmt);
   void operator()(VarDecl const &vd);
   void operator()(ASTNode const &node);
   void operator()(FuncDeclaration const &fdecl);
   void operator()(FuncCall const &fcall);

   ::std::uintmax_t current_result_ = 0;

 private:
   ::std::function<void(uintmax_t statement_result)> statement_function_;
   using varval_t = ::std::variant<::std::uintmax_t, FuncDeclaration *>;
   struct stackframe_t {
      explicit stackframe_t(StatementList const *ctx)
           : context_(ctx), var_values_(ctx->var_declarations_.size(), varval_t{0U})
      {
         this->create();
      }
      StatementList const *context_ = nullptr;
      ::std::vector<varval_t> var_values_;
      ~stackframe_t() { this->destroy(); }

    private:
      void create();
      void destroy();
   };
   ::std::vector<stackframe_t> stack_;
   using stackidx_t = decltype(stack_)::size_type;

   ::std::pair<stackidx_t, StatementList::varidx_t>
   find_identifier(Identifier const &id);
};

} // namespace Parser

#endif //PARSER_HPP

//
// Created by hopper on 8/18/25.
//

#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <cstdint>
#include <string>
#include <utility>
#include "tokens.hpp"

namespace Parser {

// Here is a sort of pseudo-BNF for what's being parsed.
//
// sequence = statement ; [ sequence ]
// statement = expression | identifer = expression
// expression <- expression | expression ( && | || ) boolterm
// boolterm = term | boolterm ( + | - ) term
// term <- factor | term ( * | / ) factor
// factor <- identifer | numeric_literal | "(" expression ")"


class ParseNode {
public:
   virtual ~ParseNode() = default;

   virtual ::std::uintmax_t evaluate() const = 0;
   virtual ::std::string to_infix_string() const = 0;
   virtual ::std::string to_prefix_string() const = 0;
};

using exprptr_t = ::std::unique_ptr<ParseNode>;
using parse_result_t = ::std::pair<exprptr_t, Tokens::toklist_t::iterator>;

parse_result_t
parse_statement_list(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_expression(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_boolterm(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_term(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);
parse_result_t
parse_factor(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish);

} // namespace Parser

#endif //PARSER_HPP

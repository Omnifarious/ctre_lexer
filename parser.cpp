// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include <deque>
#include <sstream>
#include <array>
#include "parser.hpp"
#include "tokens.hpp"

using namespace ::std::string_view_literals;

namespace {
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
   current_result_ = 0U;
   auto lvalue = find_identifier(id);
   auto const &varval = stack_[lvalue.first].var_values_[lvalue.second];
   assert(::std::holds_alternative<::std::uintmax_t>(varval) && "This program should be impossible.");
   current_result_ = ::std::get<::std::uintmax_t>(varval);
}

void SimpleEvaluator::operator()(NumericLiteral const &nl)
{
   current_result_ = nl.value_;
}

void SimpleEvaluator::operator()(StatementList const &sl)
{
   current_result_ = 0U;
   if (!sl.var_declarations_.empty()) {
      stack_.emplace_back(&sl);
      assert(
         stack_.back().context_->var_declarations_.size() == \
         sl.var_declarations_.size()
         && "Mismatch between declared variables and initialized values"
      );
   }
   assert(func_args_.empty() || sl.var_declarations_.size() >= func_args_.size());
   if (!func_args_.empty()) {
      auto &topvars = stack_.back().var_values_;
      ::std::copy(func_args_.begin(), func_args_.end(), topvars.begin());
      func_args_.clear();
   }
   for (auto const &statement: sl.statements_) {
      ::std::visit(*this, *statement);
      if (statement_function_) {
         statement_function_(current_result_);
      }
   }
   if (!sl.var_declarations_.empty()) {
      assert(!stack_.empty());
      stack_.pop_back();
   }
}

void SimpleEvaluator::operator()(AssignmentStatement const &as)
{
   auto lvalue = find_identifier(as.identifier_);
   ::std::visit(*this, *as.expression_);
   stack_[lvalue.first].var_values_[lvalue.second] = current_result_;
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

void SimpleEvaluator::operator()(WhileStatement const &whilestmt)
{
   ::std::visit(*this, *whilestmt.condition_);
   auto cond = current_result_;
   while (cond) {
      ::std::visit(*this, *whilestmt.repeated_);
      ::std::visit(*this, *whilestmt.condition_);
      cond = current_result_;
   }
}

void SimpleEvaluator::operator()(VarDecl const &vd)
{
   auto lvalue = find_identifier(vd.identifier_);
   ::std::visit(*this, *vd.expression_);
   stack_[lvalue.first].var_values_[lvalue.second] = current_result_;
   current_result_ = 0;
}

void SimpleEvaluator::operator ()(FuncDeclaration const &fdecl) {
   auto const &idinfo = find_identifier(fdecl.name_);
   assert(idinfo.first == 0);
   assert(idinfo.first < stack_.size());
   assert(idinfo.second < stack_[idinfo.first].var_values_.size());
   stack_[idinfo.first].var_values_[idinfo.second] = varval_t{&fdecl};
}

void SimpleEvaluator::operator()(FuncCall const &fcall) {
   auto const &idinfo = find_identifier(fcall.func_);
   assert(idinfo.first < stack_.size());
   assert(idinfo.second < stack_[idinfo.first].var_values_.size());
   auto const &var_ref = stack_[idinfo.first].var_values_[idinfo.second];
   assert(::std::holds_alternative<FuncDeclaration const *>(var_ref));
   auto const * const funcdecl = ::std::get<FuncDeclaration const *>(var_ref);
   assert(funcdecl != nullptr);
   ::std::vector<varval_t> local_args;
   local_args.reserve(fcall.param_exprs_.size());
   for (auto const &arg : fcall.param_exprs_) {
      ::std::visit(*this, *arg);
      local_args.emplace_back(current_result_);
      current_result_ = 0;
   }
   assert(funcdecl->body_ != nullptr);
   assert(func_args_.empty());
   assert(local_args.size() == fcall.param_exprs_.size());
   func_args_ = ::std::move(local_args);
   ::std::visit(*this, *funcdecl->body_);
   assert(func_args_.empty());
}

std::pair<SimpleEvaluator::stackidx_t, StatementList::varidx_t>
SimpleEvaluator::find_identifier(Identifier const &id)
{
   for (stackidx_t frame = stack_.size(); frame > 0;) {
      --frame;
      if (stack_[frame].context_ == id.scope_) {
         return {frame, id.varidx_};
      }
   }
   assert(false && "Identifier context not found in any stack frame");
   return {0, 0};
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
      result_ << id.scope_->var_declarations_[id.varidx_].name_;
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
      auto const &id = as.identifier_;
      result_ << id.scope_->var_declarations_[id.varidx_].name_ << " = ";
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

   void operator()(WhileStatement const &whilestmt)
   {
      result_ << "while (";
      ::std::visit(*this, *whilestmt.condition_);
      result_ << ") {\n";
      ::std::visit(*this, *whilestmt.repeated_);
      result_ << ";\n}\n";
   }

   void operator()(VarDecl const &vd)
   {
      auto const &id = vd.identifier_;
      result_ << "var "
              << id.scope_->var_declarations_[id.varidx_].name_
              << " = ";
      ::std::visit(*this, *vd.expression_);
   }

   void operator ()(FuncDeclaration const &fdecl) {
      return;  // TODO do _something_ rather than nothing.
   }
   void operator()(FuncCall const &fcall) {
      return;  // TODO do _something_ rather than nothing.
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
      result_ << id.scope_->var_declarations_[id.varidx_].name_;
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
      auto const &id = as.identifier_;
      result_ << "(setq "
              << id.scope_->var_declarations_[id.varidx_].name_
              << " ";
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

   void operator()(WhileStatement const &whilestmt)
   {
      result_ << "(while (";
      ::std::visit(*this, *whilestmt.condition_);
      result_ << ") (progn\n";
      ::std::visit(*this, *whilestmt.repeated_);
      result_ << "))\n";
   }

   void operator()(VarDecl const &vd)
   {
      auto const &id = vd.identifier_;
      result_ << "(setq-new "
              << id.scope_->var_declarations_[id.varidx_].name_
              << ' ';
      ::std::visit(*this, *vd.expression_);
      result_ << ")";
   }

   void operator()(FuncDeclaration const &fdecl) {
      auto const &id = fdecl.name_;
      result_ << "(defun ";
      result_ << id.scope_->var_declarations_[id.varidx_].name_;;
      result_ << " (";
      auto const *slist = get_if<StatementList>(fdecl.body_.get());
      assert(slist && "Function somehow missing a body!");
      assert(
         slist->var_declarations_.size() >= fdecl.num_args_ &&
         "Function body has fewer parameters than declared!"
      );
      for (
         StatementList::varidx_t paramidx = 0;
         paramidx < fdecl.num_args_;
         ++paramidx
      ) {
         if (paramidx != 0) {
            result_ << " ";
         }
         result_ << slist->var_declarations_[paramidx].name_;
      }
      result_ << ")\n";
      ::std::visit(*this, *fdecl.body_);
      result_ << ")";
   }

   void operator()(FuncCall const &fcall) {
      result_ << "(";
      (*this)(fcall.func_);
      for (auto const &arg : fcall.param_exprs_) {
         result_ << " ";
         ::std::visit(*this, *arg);
      }
      result_ << ")\n";
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

namespace priv_ {
class parse_context {
public:
   ::std::vector<StatementList *> block_stack_;

   ::std::optional<StatementList::varidx_t>
   declare_var(
      Tokens::Identifier const &id,
      VarInfo::vartype_t type = VarInfo::UInt64
   ) const {
      auto * const curscope = block_stack_.back();
      auto &scopedecls = curscope->var_declarations_;
      auto const declpos =
         ::std::find_if(scopedecls.begin(), scopedecls.end(),
            [&id](VarInfo const &a) -> bool {
               return a.name_ == id.value_;
            });
      if (declpos != scopedecls.end()) {
         ::std::cerr << "Redeclaration of variable " << id.value_ << "!\n";
         return {};
      }
      scopedecls.emplace_back(id.value_, type);
      return {scopedecls.size() - 1};
   }
};

class scope_frame {
 public:
   scope_frame(parse_context &ctx, StatementList *block) : ctx_(ctx)
   {
      ctx_.block_stack_.push_back(block);
   }

   scope_frame(const scope_frame &) = delete;
   scope_frame &operator=(const scope_frame &) = delete;
   scope_frame(scope_frame &&) = delete;
   scope_frame &operator=(scope_frame &&) = delete;

   ~scope_frame()
   {
      pop_now();
   }

   void pop_now()
   {
      if (!popped_) {
         ctx_.block_stack_.pop_back();
         popped_ = true;
      }
   }

 private:
   bool popped_ = false;
   parse_context &ctx_;

   void *operator new(size_t size) = delete;
};
}

using namespace priv_;

parse_result_t parse_top(Tokens::toklist_t::iterator start, Tokens::toklist_t::iterator finish)
{
   parse_context ctx;
   return parse_new_scope(start, finish, ctx);
}

parse_result_t parse_new_scope(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
) {
   auto statements_node = ::std::make_unique<ASTNode>(StatementList{});
   scope_frame frame(ctx, &::std::get<StatementList>(*statements_node));
   return parse_sequence(start, finish, ctx, ::std::move(statements_node));
}

parse_result_t parse_sequence(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx,
   astptr_t slist_node
)
{
   auto &statements = ::std::get<StatementList>(*slist_node);

   while (start != finish) {
      auto [statement, remainder] = parse_statement(start, finish, ctx);

      if (!statement) {
         break;
      }
      start = remainder;
      statements.statements_.emplace_back(::std::move(statement));
   }
   return parse_result_t{::std::move(slist_node), start};
}

parse_result_t parse_statement(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
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
      auto [stmt, remainder] = parse_new_scope(start, finish, ctx);
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
      auto [stmt, remainder] = parse_ifelse(start, finish, ctx);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_while(start, finish, ctx);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_vardecl(start, finish, ctx);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_func_declaration(start, finish, ctx);
      stmt
   ) {
      if (ctx.block_stack_.size() >= 2) {
         ::std::cerr << "Function declaration must be at the top level!\n";
         return parse_result_t{nullptr, finish};
      }
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_assignment(start, finish, ctx);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   } else if (
      auto [stmt, remainder] = parse_expression_statement(start, finish, ctx);
      stmt
   ) {
      statement = std::move(stmt);
      after = remainder;
   }
   return parse_result_t{::std::move(statement), after};
}

parse_result_t parse_while(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   // While
   if (
      auto const *kw = ::std::get_if<Tokens::Keyword>(&(*start));
      !(kw && kw->value_ == Tokens::Keyword::While)
   ) {
      // Not a while
      return parse_result_t{nullptr, finish};
   }

   start = ::std::next(start);
   if (
      start == finish ||
      !::std::holds_alternative<Tokens::Paren>(*start) ||
      ::std::get<Tokens::Paren>(*start).value_ != Tokens::Paren::Open
   ) {
      ::std::cerr << "Expected ( after while, aborting!\n";
      return parse_result_t{nullptr, finish};
   }

   start = ::std::next(start);
   auto [expr, after_exp] = parse_expression(start, finish, ctx);
   if (!expr) {
      ::std::cerr << "Expected expression after while, aborting!\n";
      return parse_result_t{nullptr, finish};
   }

   start = after_exp;
   if (
      start == finish ||
      !::std::holds_alternative<Tokens::Paren>(*start) ||
      ::std::get<Tokens::Paren>(*start).value_ != Tokens::Paren::Close
   ) {
      ::std::cerr << "Expected ) after while, aborting!\n";
   }

   start = ::std::next(start);
   auto [repeated, after_repeated] = parse_statement(start, finish, ctx);
   if (!repeated) {
      ::std::cerr << "Expected statement after while, aborting!\n";
      return parse_result_t{nullptr, finish};
   }
   astptr_t while_statement = ::std::make_unique<ASTNode>(
      WhileStatement{::std::move(expr), ::std::move(repeated)}
   );
   return parse_result_t{
      ::std::move(while_statement),
      after_repeated
   };
}

parse_result_t parse_vardecl(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   // Look for var keyword
   if (
      auto const *kw = ::std::get_if<Tokens::Keyword>(&(*start));
      !(kw && kw->value_ == Tokens::Keyword::Var)
   ) {
      // Not a var keyword
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   auto const id = ::std::get_if<Tokens::Identifier>(&(*start));
   if (!id) {
      return parse_result_t{nullptr, finish};
   }
   auto const &idtok = *id;
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *eq = ::std::get_if<Tokens::Operator>(&(*start));
      !(eq && eq->value_ == Tokens::Operator::Equal)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   auto [expr, remainder] = parse_expression(start, finish, ctx);
   if (!expr) {
      return parse_result_t{nullptr, finish};
   }
   if (
      remainder == finish ||
      !::std::holds_alternative<Tokens::Semicolon>(*remainder)
   ) {
      return parse_result_t{nullptr, finish};
   }

   assert(!ctx.block_stack_.empty());
   // Failure means variable was already declared.
   auto const varidx = ctx.declare_var(idtok);
   if (!varidx.has_value()) {
      return parse_result_t{nullptr, finish};
   }
   return parse_result_t{
      ::std::make_unique<ASTNode>(
         VarDecl{
            Identifier{ctx.block_stack_.back(), varidx.value()},
            ::std::move(expr)
         }
      ),
      ::std::next(remainder)
   };
}

parse_result_t parse_func_declaration(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *kw = ::std::get_if<Tokens::Keyword>(&(*start));
      !(kw && kw->value_ == Tokens::Keyword::Def)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   auto const *funcname = ::std::get_if<Tokens::Identifier>(&(*start));
   if (!funcname) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Open)
   ) {
      return parse_result_t{nullptr, finish};
   }

   ::std::vector<Tokens::Identifier> args;

   start = ::std::next(start);
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (auto const *arg = ::std::get_if<Tokens::Identifier>(&(*start))) {
      args.emplace_back(*arg);
      start = ::std::next(start);
      if (start == finish) {
         return parse_result_t{nullptr, finish};
      }
      auto const next_is_comma = [&] {
         auto const *comma = ::std::get_if<Tokens::Punctuator>(&(*start));
         return comma && comma->value_ == Tokens::Punctuator::Comma;
      };
      bool found_comma = next_is_comma();
      while (found_comma) {
         start = ::std::next(start);
         if (start == finish) {
            return parse_result_t{nullptr, finish};
         }
         if (auto const *arg = ::std::get_if<Tokens::Identifier>(&(*start))) {
            args.emplace_back(*arg);
         } else {
            return parse_result_t{nullptr, finish};
         }
         start = ::std::next(start);
         if (start == finish) {
            return parse_result_t{nullptr, finish};
         }
         found_comma = next_is_comma();
      }
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Close)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *brace = ::std::get_if<Tokens::CurlyBracket>(&(*start));
      !(brace && brace->value_ == Tokens::CurlyBracket::Open)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   auto varidx = ctx.declare_var(*funcname, VarInfo::Function);
   if (!varidx.has_value()) {
      return parse_result_t{nullptr, finish};
   }
   auto const func_scope = ctx.block_stack_.back();

   auto slist_node = ::std::make_unique<ASTNode>(StatementList{});
   auto &slist = ::std::get<StatementList>(*slist_node);
   scope_frame frame{ctx, &slist};
   for (auto const &arg : args) {
      auto argidx = ctx.declare_var(arg, VarInfo::UInt64);
      if (!argidx.has_value()) {
         ::std::cerr << "Argument name " << arg.value_ << " used twice!\n";
         return parse_result_t{nullptr, finish};
      }
   }
   assert(slist.var_declarations_.size() == args.size());
   auto [body, remainder] = parse_sequence(start, finish, ctx, ::std::move(slist_node));
   if (!body) {
      return parse_result_t{nullptr, finish};
   }
   if (remainder == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *brace = ::std::get_if<Tokens::CurlyBracket>(&(*remainder));
      !(brace && brace->value_ == Tokens::CurlyBracket::Close)
   ) {
      return parse_result_t{nullptr, finish};
   }

   return parse_result_t{
      ::std::make_unique<ASTNode>(
         FuncDeclaration{
            Identifier{func_scope, varidx.value()},
            ::std::move(body),
            args.size()
         }
      ),
      ::std::next(remainder)
   };
}

parse_result_t parse_func_call(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &context)
{
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   auto [funcid, afterfuncname] = parse_identifier(start, finish, context);
   if (!funcid) {
      return parse_result_t{nullptr, finish};
   }
   assert(::std::holds_alternative<Identifier>(*funcid));
   auto &funcname = ::std::get<Identifier>(*funcid);
   if (funcname.scope_->var_declarations_[funcname.varidx_].type_ != VarInfo::Function) {
      return parse_result_t{nullptr, finish};
   }
   start = afterfuncname;

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Open)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }

   ::std::vector<astptr_t> args;

   auto [expr, remainder] = parse_expression(start, finish, context);
   while (expr) {
      args.emplace_back(::std::move(expr));

      start = remainder;
      if (start == finish) {
         return parse_result_t{nullptr, finish};
      }
      if (
         auto const *comma = ::std::get_if<Tokens::Punctuator>(&(*start));
         !(comma && comma->value_ == Tokens::Punctuator::Comma)
      ) {
         break;
      }
      start = ::std::next(start);

      auto [nextexpr, afterexpr] = parse_expression(start, finish, context);
      if (!nextexpr) {
         return parse_result_t{nullptr, finish};
      }
      expr = ::std::move(nextexpr);
      remainder = afterexpr;
   }

   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   if (
      auto const *paren = ::std::get_if<Tokens::Paren>(&(*start));
      !(paren && paren->value_ == Tokens::Paren::Close)
   ) {
      return parse_result_t{nullptr, finish};
   }
   start = ::std::next(start);

   return {
      ::std::make_unique<ASTNode>(
         FuncCall{funcname, ::std::move(args)}
      ),
      start
   };
}

parse_result_t parse_assignment(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   auto [id, after_id] = parse_identifier(start, finish, ctx);
   if (!id) {
      return {nullptr, finish};
   }
   if (after_id == finish) {
      return {nullptr, finish};
   }
   if (
      !(::std::holds_alternative<Tokens::Operator>(*after_id) &&
        ::std::get<Tokens::Operator>(*after_id).value_ == Tokens::Operator::Equal)
   ) {
      return {nullptr, finish};
   }
   auto const after_eq = ::std::next(after_id);
   if (after_eq == finish) {
      return {nullptr, finish};
   }
   auto [expr, remainder] =
         parse_expression(after_eq, finish, ctx);
   if (!expr) {
      ::std::cerr << "Didn't find expression after = in "
            "assignment, aborting!\n";
      return {nullptr, finish};
   }
   if (
      remainder == finish ||
      !::std::holds_alternative<Tokens::Semicolon>(*remainder)
   ) {
      ::std::cerr << "Didn't find semicolon after assignment, "
            "aborting!\n";
      return {nullptr, finish};
   }
   assert(::std::holds_alternative<Identifier>(*id));
   return {
      ::std::make_unique<ASTNode>(
         AssignmentStatement{::std::get<Identifier>(*id), ::std::move(expr)}
      ),
      ::std::next(remainder)
   };
}

parse_result_t parse_ifelse(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
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
   auto [expr, after_exp] = parse_expression(start, finish, ctx);
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
   auto [then_block, after_then] = parse_statement(start, finish, ctx);
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
   auto [else_block, after_else] = parse_statement(start, finish, ctx);
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
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   auto [expr, remainder] = parse_expression(start, finish, ctx);
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
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   ::std::deque<astptr_t> boolterms;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (boolterms.size() == operators.size()) {
      auto [boolterm, remainder] = parse_boolterm(start, finish, ctx);
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
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   ::std::deque<astptr_t> boolterms;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (true) {
      auto [relclause, remainder] = parse_relclause(start, finish, ctx);
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
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   ::std::deque<astptr_t> relclauses;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return parse_result_t{nullptr, finish};
   }
   while (true) {
      auto [term, remainder] = parse_term(start, finish, ctx);
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
parse_term(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   ::std::deque<astptr_t> factors;
   ::std::deque<Tokens::Operator> operators;
   if (start == finish) {
      return {nullptr, finish};
   }
   while (true) {
      auto [factor, remainder] = parse_factor(start, finish, ctx);
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

parse_result_t parse_factor(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   if (start == finish) {
      return {nullptr, finish};
   }
   if (
      auto func_call_result = parse_func_call(start, finish, ctx);
      func_call_result.first
   ) {
      return func_call_result;
   }
   if (auto idresult = parse_identifier(start, finish, ctx); idresult.first) {
      return idresult;
   }
   auto const &tok = *start;
   if (auto const num = ::std::get_if<Tokens::UnsignedInteger>(&tok)) {
      return {
         ::std::make_unique<ASTNode>(NumericLiteral{num->value_}),
         ::std::next(start)
      };
   } else if (auto const op = ::std::get_if<Tokens::Paren>(&tok)) {
      if (op->value_ == Tokens::Paren::Open) {
         ++start;
         auto [expr, remainder] = parse_boolterm(start, finish, ctx);
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

parse_result_t
parse_identifier(
   Tokens::toklist_t::iterator start,
   Tokens::toklist_t::iterator finish,
   parse_context &ctx
)
{
   if (start == finish) {
      return {nullptr, finish};
   }
   auto const id = ::std::get_if<Tokens::Identifier>(&*start);
   if (!id) {
      return {nullptr, finish};
   }
   for (auto slist_: ::std::ranges::reverse_view{ctx.block_stack_}) {
      auto &vdecls = slist_->var_declarations_;
      auto idloc =
         ::std::find_if(
            vdecls.begin(), vdecls.end(),
            [&id](VarInfo const &a) -> bool {
               return a.name_ == id->value_;
         });
      if (idloc != vdecls.end()) {
         auto const ididx = ::std::distance(vdecls.begin(), idloc);
         assert(ididx >= 0);
         return {
            ::std::make_unique<ASTNode>(
               Identifier{slist_, static_cast<StatementList::varidx_t>(ididx)}
            ),
            ::std::next(start)
         };
      }
   }
   ::std::cerr << "Identifier " << id->value_ << " not found!\n";
   return {nullptr, finish};
}

} // namespace Parser

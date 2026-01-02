// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "tokens.hpp"
#include <iterator>
#include <concepts>
#include <type_traits>

namespace Tokens::priv_ {

template <class T, class... Us>
concept one_of = (::std::same_as<::std::remove_cvref_t<T>, Us> || ...);

template <class T>
concept HasPrintableValue =
   one_of<T, UnsignedInteger, Identifier, Semicolon>;

template <class T>
concept HasEnumValue =
   !HasPrintableValue<T>;

template <typename FmtContext>
struct print_visitor {
   FmtContext &ctx_;

   explicit print_visitor(FmtContext &ctx) : ctx_(ctx) {}

   template <HasPrintableValue TokType>
   FmtContext::iterator operator ()(TokType const &t) {
      return ::std::format_to(ctx_.out(), "{}({})", t.S_token_name, t.value_);
   }
   template <HasEnumValue TokType>
   FmtContext::iterator operator ()(TokType const &t) {
      return ::std::format_to(
         ctx_.out(), "{}({})", t.S_token_name, t.S_val_str[t.value_]
      );
   }
};
} // namespace priv_

using tokformatter_t = ::std::formatter<Tokens::AnyToken, char>;
using fmtctx_t = ::std::format_context;

fmtctx_t::iterator
tokformatter_t::format(Tokens::AnyToken const &t, fmtctx_t &ctx ) const
{
   return ::std::visit(::Tokens::priv_::print_visitor<fmtctx_t>{ctx}, t);
}

namespace Tokens {
::std::ostream &operator <<(::std::ostream &os, Tokens::AnyToken const &a)
{
   ::std::format_to(::std::ostream_iterator<char>(os), "{}", a);
   return os;
}
} // namespace Tokens

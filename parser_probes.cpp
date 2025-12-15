// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "parser.hpp"
#include <iostream>

using ::std::cerr;

void Parser::SimpleEvaluator::stackframe_t::create()
{
   cerr << "New frame at " << this << "\n";
}

void Parser::SimpleEvaluator::stackframe_t::destroy()
{
   cerr << "Destroying frame at " << this << "\n";
}

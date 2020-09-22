// -*- mode: c++ -*-

#include <ctre.hpp>

int main(int argc, char const *argv[])
{
   if (argc == 2) {
      static constexpr auto pattern = ctll::fixed_string{ "Login:\\s+$" };
      if (ctre::match<pattern>(argv[1])) {
         return 0;
      }
   }
   return 1;
}

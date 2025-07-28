#include "ring_buffer_view.hpp"
#include <vector>
#include <string>
#include <iostream>

void dummy()
{
   using iter_t = decltype(adapter)::iterator;
   iter_t finger = adapter.begin();
   iter_t finished = adapter.end();
   iter_t first_nonspace = finger;
   enum { In_Space, In_NonSpace } state = In_Space;
   ::std::vector<::std::string> words;
   for (; finger != finished; ++finger) {
      if (isspace(*finger)) {
         if (state == In_NonSpace) {
            words.emplace_back(first_nonspace, finger);
         }
         state = In_Space;
      } else {
         if (state == In_Space) {
            first_nonspace = finger;
            state = In_NonSpace;
         }
      }
   }
   if (state == In_NonSpace) {
      words.emplace_back(first_nonspace, finished);
   }
   for (auto const &word: words) {
      ::std::cout << " - [" << word << "]\n";
   }
}

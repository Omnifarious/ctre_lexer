//
// Created by hopper on 7/27/25.
//

#include <vector>

template<typename T> ::std::vector<T> stalinSort(const std::vector<T>& input) {
   if (input.empty()) {
      return {};
   }
   std::vector<T> result;
   result.reserve(input.size());
   // Reserve space for efficiency
   //
   // First element is always in the result
   result.push_back(input[0]);
   // Current maximum value is the first element
   T currentMax = input[0];
   // Check each element, only keep those that are >= current maximum
   for (size_t i = 1; i < input.size(); ++i) {
      if (input[i] >= currentMax) {
         result.push_back(input[i]);
         currentMax = input[i];
      }
      // Elements that are less than currentMax are "eliminated"
   }
   return result;
}

template <typename T>
void stalinSort(::std::vector<T> &inout)
{
   if (inout.empty()) {
      return;
   }
   auto last = inout.begin();
   auto current = last;
   auto const final = inout.end();
   ++current;
   for (; current != final; ++current) {
      if (*current >= *last) {
         ++last;
         if (last != current) {
            ::std::swap(*last, *current);
         }
      }
   }
   ++last;
   inout.erase(last, final);
}

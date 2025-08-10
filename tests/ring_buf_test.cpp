#include "../ring_buffer_view.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <catch2/catch_all.hpp>


SCENARIO("The adapter and raw input iterator iterate over the same file.")
{
   GIVEN("An input file with known test data is opened twice")
   {
      const char input_file[] = "data/ringbufin.txt";
      REQUIRE(::std::filesystem::exists(input_file));
      ::std::ifstream tdata(input_file, ::std::ios::in);
      REQUIRE(tdata.is_open());
      REQUIRE(tdata.good());
      REQUIRE(tdata.peek() != EOF);
      ::std::ifstream tcheck(input_file, ::std::ios::in);
      REQUIRE(tcheck.is_open());
      REQUIRE(tcheck.good());
      REQUIRE(tcheck.peek() != EOF);
      WHEN("A ring buffer adapter wraps the input iterator for one opened instance.")
      {
         using s_iter_t = ::std::istreambuf_iterator<char>;
         auto adapter = input_to_forward_range_adapter{s_iter_t{tdata}, s_iter_t{}};
         auto check_iter = s_iter_t{tcheck};
         auto const check_end = s_iter_t{};
         AND_WHEN("Another input iterator is created for the opened instance.")
         {
            using iter_t = decltype(adapter)::iterator;
            iter_t finger = adapter.begin();
            iter_t finished = adapter.end();
            THEN("The two iterators should point at the same thing when iterated in tandem.")
            {
               while (finger != finished) {
                  REQUIRE(check_iter != check_end);
                  REQUIRE(*finger == *check_iter);
                  ++finger;
                  ++check_iter;
               }
            }
         }
      }
   }
}

/*
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
*/

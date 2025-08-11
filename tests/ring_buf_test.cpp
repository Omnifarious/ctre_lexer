#include "../ring_buffer_view.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <catch2/catch_all.hpp>

using s_iter_t = ::std::istreambuf_iterator<char>;
using adapter4  = input_to_forward_range_adapter<s_iter_t, 4>;   // 16
using adapter8  = input_to_forward_range_adapter<s_iter_t, 8>;   // 256

SCENARIO("The adapter and raw input iterator iterate over the same file.")
{
   GIVEN("An input file with known test data is opened twice")
   {
      const char input_file[] = "data/ringbufin.txt";
      REQUIRE(::std::filesystem::exists(input_file));
      REQUIRE(::std::filesystem::is_regular_file(input_file));
      REQUIRE(::std::filesystem::file_size(input_file) > 10000);
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
         AND_WHEN("Another input iterator is created for the other opened instance.")
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

SCENARIO("Empty input yields empty range")
{
   GIVEN("An empty stream")
   {
      std::istringstream ss("");
      adapter8 a{s_iter_t{ss}, s_iter_t{}};
      WHEN("begin/end are queried") {
         auto b = a.begin();
         auto e = a.end();
         THEN("begin == end") { REQUIRE(b == e); }
      }
   }
}

SCENARIO("Single element stream")
{
   GIVEN("A one-byte stream")
   {
      std::istringstream ss("X");
      adapter8 a{s_iter_t{ss}, s_iter_t{}};
      WHEN("iterating") {
         auto it = a.begin(), e = a.end();
         REQUIRE(it != e);
         REQUIRE(*it == 'X');
         ++it;
         THEN("one increment reaches end") { REQUIRE(it == e); }
      }
   }
}

SCENARIO("Wrap-around does not corrupt sequence")
{
   // bufsize=16, feed >16 to force wrap several times
   GIVEN("A small buffer adapter and >3x buffer data")
   {
      std::string payload;
      for (int i = 0; i < 64; ++i) payload.push_back(char('a' + (i % 26)));
      std::istringstream ss(payload);
      adapter4 a{s_iter_t{ss}, s_iter_t{}};

      WHEN("consuming entire range") {
         std::string out;
         for (auto c : a) out.push_back(c);
         THEN("output matches input exactly") { REQUIRE(out == payload); }
      }
   }
}

SCENARIO("Postfix ++ returns prior value")
{
   GIVEN("A short stream")
   {
      std::istringstream ss("AB");
      adapter8 a{s_iter_t{ss}, s_iter_t{}};
      auto it = a.begin();
      WHEN("post-incremented") {
         auto old = it++;
         THEN("saved iterator still derefs original element") {
            REQUIRE(*old == 'A');
            REQUIRE(*it  == 'B');
         }
      }
   }
}

SCENARIO("Iterator copy is independent and equality behaves")
{
   GIVEN("A stream with several chars")
   {
      std::istringstream ss("HELLO");
      adapter8 a{s_iter_t{ss}, s_iter_t{}};
      auto i1 = a.begin();
      auto i2 = i1; // copy

      REQUIRE(i1 == i2);
      REQUIRE(*i1 == 'H');

      WHEN("advancing one copy") {
         ++i1;
         THEN("copies compare correctly and deref is correct") {
            REQUIRE(i1 != i2);
            REQUIRE(*i1 == 'E');
            REQUIRE(*i2 == 'H');
         }
      }

      AND_WHEN("comparing ends") {
         auto e1 = a.end();
         auto e2 = a.end();
         THEN("end equals end from same adapter") { REQUIRE(e1 == e2); }
      }
   }
}

SCENARIO("Slow iterator falls behind and throws buffer_overflow_error on deref")
{
   GIVEN("Tiny buffer and two iterators over same adapter")
   {
      // bufsize=16; advance fast >16 ahead of slow
      std::string data(48, 'Z'); // 3x buffer
      std::istringstream ss(data);
      adapter4 a{s_iter_t{ss}, s_iter_t{}};

      auto slow = a.begin();
      auto fast = slow;

      // move fast beyond one full buffer so slow is stale
      for (int i = 0; i < 20; ++i) ++fast;

      WHEN("dereferencing the slow (stale) iterator") {
         THEN("buffer_overflow_error is thrown") {
            REQUIRE_THROWS_AS(*slow, buffer_overflow_error);
         }
      }
   }
}

SCENARIO("End iterator remains equal after exhausting range")
{
   GIVEN("A small stream")
   {
      std::istringstream ss("XYZ");
      adapter8 a{s_iter_t{ss}, s_iter_t{}};
      auto it = a.begin();
      auto e  = a.end();
      while (it != e) ++it;
      WHEN("incrementing beyond end") {
         ++it; // should be a no-op per your ++ logic at end_generation
         THEN("still equals end") { REQUIRE(it == e); }
      }
   }
}

SCENARIO("Multiple buffer sizes behave identically")
{
   GIVEN("Same payload through different buf sizes")
   {
      std::string payload(1000, '\x7f'); // non-ASCII to avoid accidental text logic
      std::istringstream s1(payload), s2(payload);

      adapter4 a1{s_iter_t{s1}, s_iter_t{}};
      adapter8 a2{s_iter_t{s2}, s_iter_t{}};

      std::string out1, out2;
      for (auto c : a1) out1.push_back(c);
      for (auto c : a2) out2.push_back(c);

      THEN("outputs are identical and equal to input") {
         REQUIRE(out1 == payload);
         REQUIRE(out2 == payload);
         REQUIRE(out1 == out2);
      }
   }
}

// -*- mode: c++ -*-

#include <ctre.hpp>
#include <optional>
#include <cassert>
#include <ranges>

class telnet_eater {
 public:
   bool operator()(char c) {
      auto const input = static_cast<unsigned char>(c);
      using ::std::nullopt;
      switch (parser_state) {
         case Normal:
            if (input == 0xff) {
               parser_state = Seen_IAC;
               return false;
            } else {
               return true;
            }
            break;
         case Seen_IAC:
            if (input == 0xff) {
               parser_state = Normal;
               return true;
            } else if (!(input == 0xfb || input == 0xfc || input == 0xfd || input == 0xfe)) {
               assert(false && "Unknown telnet escape sequence.");
               parser_state = Normal;
               return false;
            } else {
               parser_state = Seen_Code;
               return false;
            }
            break;
         case Seen_Code:
            parser_state = Normal;
            return false;
      }
      return false;
   }

 private:
   enum {
      Normal,
      Seen_IAC,
      Seen_Code
   } parser_state = Normal;
};

template <::std::ranges::input_range T>
auto inline operator |(T &in, telnet_eater &&eater)
{
   using ::std::ranges::views::filter;
   using ::std::move;
   return in | filter(move(eater));
}

template <::std::ranges::input_range T>
auto inline operator |(T &in, telnet_eater &eater)
{
   using ::std::ranges::views::filter;
   return in | filter(eater);
}

void remove_telnet(::std::string_view sv, void (*outputter)(char c)) {
   using ::std::ranges::views::filter;
   for (auto c: sv | telnet_eater{}) {
      outputter(c);
   }
}

::std::string remove_telnet(::std::string_view sv) {
   using ::std::ranges::views::filter;
   telnet_eater eater;
   auto filtered = sv | eater;
   return ::std::string{filtered.begin(), filtered.end()};
}


#include <coroutine>
#include <iostream>
#include <optional>
#include <iterator>
#include <concepts>

template<std::movable T>
class Generator {
 public:
   struct promise_type {
      Generator<T> get_return_object() {
         return Generator{Handle::from_promise(*this)};
      }
      static std::suspend_always initial_suspend() noexcept {
         return {};
      }
      static std::suspend_always final_suspend() noexcept {
         return {};
      }
      std::suspend_always yield_value(T value) noexcept {
         current_value = std::move(value);
         return {};
      }
      // Disallow co_await in generator coroutines.
      void await_transform() = delete;
      static void unhandled_exception() {
         throw;
      }

      std::optional<T> current_value;
   };

   using Handle = std::coroutine_handle<promise_type>;

   explicit Generator(Handle coroutine) :
           m_coroutine{coroutine}
   {}

   Generator() = default;
   ~Generator() {
      if (m_coroutine) {
         m_coroutine.destroy();
      }
   }

   Generator(const Generator&) = delete;
   Generator& operator=(const Generator&) = delete;

   Generator(Generator&& other) noexcept :
           m_coroutine{other.m_coroutine}
   {
      other.m_coroutine = {};
   }

   Generator& operator=(Generator&& other) noexcept {
      if (this != &other) {
         if (m_coroutine) {
            m_coroutine.destroy();
         }
         m_coroutine = other.m_coroutine;
         other.m_coroutine = {};
      }
      return *this;
   }

   // Range-based for loop support.
   class Iter {
    public:
      void operator++() {
         m_coroutine.resume();
      }
      const T& operator*() const {
         return *m_coroutine.promise().current_value;
      }
      bool operator==(std::default_sentinel_t) const {
         return !m_coroutine || m_coroutine.done();
      }

      explicit Iter(Handle coroutine) :
              m_coroutine{coroutine}
      {}

    private:
      Handle m_coroutine;
   };

   Iter begin() {
      if (m_coroutine) {
         m_coroutine.resume();
      }
      return Iter{m_coroutine};
   }
   std::default_sentinel_t end() {
      return {};
   }

 private:
   Handle m_coroutine;
};

template<std::integral T>
Generator<T> range(T first, T last) {
   while (first < last) {
      co_yield first++;
   }
}

void print_range(::std::ostream &os, int start, int end)
{
   for (auto i: range(start, end)) {
      os << i << '\n';
   }
}

int main()
{
   print_range(::std::cout, -5, 5);
}

#if 0
int main(int argc, char const *argv[])
{
   using sv = ::std::string_view;
   if (argc == 2) {
      static constexpr auto pattern = ctll::fixed_string{ "Login:\\s+$" };
      auto arg1 = sv{argv[1]};
      telnet_eater eater;
      auto filtered = arg1 | eater;
      if (ctre::match<pattern>(filtered.begin(), filtered.end())) {
         return 0;
      }
   }
   return 1;
}
#endif

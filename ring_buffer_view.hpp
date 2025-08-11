#pragma once

// Copyright © 2025 Eric Hopper. All rights reserved.
// Created by hopper on 6/30/25.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <array>
#include <iterator>
#include <limits>
#include <cassert>

#ifndef CHUNK_RANGE_HPP
#define CHUNK_RANGE_HPP

class buffer_overflow_error : public ::std::runtime_error {
 public:
   buffer_overflow_error() : runtime_error("Ring buffer overflow.")
   {
   }
};

template<::std::input_iterator I, ::std::uint8_t size_log2 = 13>
class input_to_forward_range_adapter {
   static_assert(size_log2 >= 4 && size_log2 <= 28,
                 "Buffer must be from 2^4 to 2^28 in size.");

public:
   struct iterator;
   friend struct iterator;
   using value_type = typename I::value_type;
   static constexpr auto bufsize = ::std::size_t{1U} << size_log2;
   using gen_t = ::std::uint64_t;
   static constexpr auto end_generation = ::std::numeric_limits<gen_t>::max();

   explicit input_to_forward_range_adapter(I const &start, I const &end)
      : input_(start), end_(end)
   {
      next_character();
   }

   struct iterator {
      friend class input_to_forward_range_adapter;
      using difference_type = ::std::ptrdiff_t;
      using value_type = input_to_forward_range_adapter::value_type;
      using iterator_category = ::std::forward_iterator_tag;
      using reference = value_type const &;

      iterator() = default;

      value_type operator *() const
      {
         auto const &p = *parent_;
         if (generation_ == end_generation) {
            ::std::unreachable();
         }
         if (generation_ == p.generation_ && pos_ < p.write_pos_) {
            return p.buffer_[pos_];
         } else if (generation_ < p.generation_ && pos_ >= p.write_pos_) {
            return p.buffer_[pos_];
         } else {
            throw buffer_overflow_error();
         }
      }

      iterator &operator ++()
      {
         auto &p = *parent_;
         if (generation_ == end_generation) {
            return *this;
         }
         ++pos_;
         if (pos_ >= bufsize) {
            pos_ = 0;
            ++generation_;
         }
         if (generation_ > p.generation_ ||
             generation_ == p.generation_ && pos_ >= p.write_pos_) {
            if (!p.next_character()) {
               generation_ = end_generation;
               return *this;
            }
         }
         assert(
            generation_ == p.generation_ && (generation_ == end_generation || pos_ < p.write_pos_) ||
            generation_ < p.generation_
         );
         return *this;
      }

      iterator operator ++(int)
      {
         iterator saved{*this};
         ++(*this);
         return saved;
      }

      iterator &operator =(iterator const &b) = default;

      bool operator ==(iterator const &b) const
      {
         if (parent_ == nullptr) {
            return false;
         }
         if (generation_ == end_generation && b.generation_ == end_generation) {
            return true;
         }
         if (parent_ != b.parent_) {
            return false;
         } else if (generation_ != b.generation_) {
            return false;
         } else if (pos_ != b.pos_) {
            return false;
         }
         return true;
      }

      bool operator !=(iterator const &b) const
      {
         return !operator ==(b);
      }

   protected:
      iterator(
         input_to_forward_range_adapter *parent,
         ::std::uint64_t generation, ::std::size_t pos
      )
         : parent_(parent), generation_(generation), pos_(pos)
      {
         assert(parent != nullptr);
      }

   private:
      input_to_forward_range_adapter *parent_ = nullptr;
      ::std::uint64_t generation_ = end_generation;
      ::std::size_t pos_ = 0U;
   };

   iterator begin()
   {
      // (write_pos_ == 0 && generation_ == 0) means the first character
      // couldn't be read and the file is empty.
      if (input_ != end_ || !(write_pos_ == 0 && generation_ == 0)) {
         return iterator(this, 0, 0);
      } else {
         return end();
      }
   }

   iterator end()
   {
      return iterator(this, end_generation, 0);
   }

private:
   static constexpr auto S_mask = bufsize - 1;
   using buf_t = ::std::array<value_type, bufsize>;
   using bufsize_t = typename buf_t::size_type;
   buf_t buffer_;
   bufsize_t write_pos_ = 0;
   ::std::uint64_t generation_ = 0;
   I input_;
   I const end_;

   bool next_character()
   {
      if (input_ == end_) {
         return false;
      }
      if (write_pos_ >= bufsize) {
         write_pos_ = 0;
         ++generation_;
      }
      buffer_[write_pos_++] = *input_;
      ++input_;
      return true;
   }
};

#endif //CHUNK_RANGE_HPP

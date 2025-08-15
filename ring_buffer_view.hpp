#pragma once

// Copyright © 2025 Eric Hopper. All rights reserved.
// Created by hopper on 6/30/25.
// Licensed under the GNU General Public License v3.0 - see LICENSE file.

#include <iostream>
#include <cstdint>
#include <ranges>
#include <array>
#include <iterator>
#include <limits>
#include <cassert>

#ifndef RING_BUFFER_VIEW_HPP
#define RING_BUFFER_VIEW_HPP

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
   using size_type = ::std::uint64_t; // 2**64 - 1
   using difference_type = ::std::int64_t;  //  -2**63 ..  2**63 - 1
   static constexpr auto bufsize = size_type{1U} << size_log2;
   static constexpr auto sentinel_end = ::std::numeric_limits<size_type>::max();

   explicit input_to_forward_range_adapter(I const &start, I const &end)
      : input_(start), end_(end)
   {
      next_character();
   }

   struct iterator {
      friend class input_to_forward_range_adapter;
      using value_type = input_to_forward_range_adapter::value_type;
      using iterator_category = ::std::forward_iterator_tag;
      using reference = value_type const &;
      using pointer = value_type const *;
      using difference_type = input_to_forward_range_adapter::difference_type;

      iterator() = default;

      reference operator *() const
      {
         auto const &p = *parent_;
         if (combined_pos_ == sentinel_end) {
            ::std::unreachable();
         }
         if (p.combined_pos_ - combined_pos_ < bufsize) {
            return p.buffer_[read_pos()];
         } else {
            throw buffer_overflow_error();
         }
      }

      pointer operator ->() const
      {
         return &operator *();
      }

      difference_type operator -(iterator const &b) const
      {
         if (parent_ != b.parent_) {
            ::std::unreachable();
         }
         return combined_pos_ - b.combined_pos_;
      }

      iterator &operator ++()
      {
         auto &p = *parent_;
         if (combined_pos_ == sentinel_end) {
            return *this;
         }
         if (++combined_pos_ >= p.combined_pos_) {
            if (!p.next_character()) {
               combined_pos_ = sentinel_end;
               return *this;
            }
         }
         assert(
            combined_pos_ < p.combined_pos_
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
         if (combined_pos_ == sentinel_end && b.combined_pos_ == sentinel_end) {
            return true;
         }
         if (parent_ != b.parent_) {
            return false;
         }
         return combined_pos_ == b.combined_pos_;
      }

      bool operator !=(iterator const &b) const
      {
         return !operator ==(b);
      }

   protected:
      iterator(input_to_forward_range_adapter *parent, size_type combined_pos)
         : parent_(parent), combined_pos_(combined_pos)
      {
         assert(parent != nullptr);
      }

   private:
      input_to_forward_range_adapter *parent_ = nullptr;
      size_type combined_pos_ = sentinel_end;

      size_type generation() const
      {
         return combined_pos_ >> size_log2;
      }
      size_type read_pos() const
      {
         return combined_pos_ & S_mask;
      }
   };

   iterator begin()
   {
      // combined_pos_ == 0 means the file is empty because it wasn't able to
      // even read the first character.
      if (input_ != end_ || combined_pos_ != 0) {
         return iterator(this, 0U);
      } else {
         return end();
      }
   }

   iterator end()
   {
      return iterator(this, sentinel_end);
   }

private:
   static constexpr auto S_mask = bufsize - 1;
   using buf_t = ::std::array<value_type, bufsize>;
   buf_t buffer_;
   size_type combined_pos_ = 0;
   I input_;
   I const end_;

   size_type generation() const
   {
      return combined_pos_ >> size_log2;
   }
   size_type write_pos() const
   {
      return combined_pos_ & S_mask;
   }
   bool next_character()
   {
      if (input_ == end_) {
         return false;
      }
      buffer_[write_pos()] = *input_++;
      ++combined_pos_;
      if (combined_pos_ > ::std::numeric_limits<difference_type>::max()) {
         throw ::std::overflow_error("Too many input characters.");
      }
      return true;
   }
};

#endif //CHUNK_RANGE_HPP

/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * mopo is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

/**
 * @file circular_queue.h
 * @brief Implémentation d'une file circulaire lock-free adaptée au code audio temps réel.
 *
 * Template simple pour un producteur / consommateur unique, sans allocations
 * après construction (sauf via reserve). Conçue pour passages d'événements
 * entre threads (UI ? audio) avec latence minimale.
 */

#include "processor.h"
#include "utils.h"

#include <cstddef>
#include <concepts>
#include <atomic>

namespace mopo {

  /**
   * @brief Lock-free circular queue for real-time audio processing.
   * @ingroup mopo_modules
   * @tparam T Type of elements stored in the queue. Must be Copyable and
   *           EqualityComparable because several operations rely on copying
   *   and equality checking (remove, removeAll, count).
   *
   * This class implements a single-producer / single-consumer circular buffer
   * intended for use in low-latency audio code (for example passing events
   * from a UI or MIDI thread to the audio thread). It uses a fixed-size
   * contiguous array internally and atomics for the read and write indices
   * (`start_` and `end_`) to avoid locks.
   *
   * Important usage notes:
   * - Designed for one writer thread and one reader thread. Concurrent
   *   multiple-writer or multiple-reader scenarios are not supported.
   * - No dynamic allocations occur after construction or `reserve()`; this
   *   makes it safe for real-time contexts.
   * - Indices use `std::atomic<int>` with `memory_order_acquire`/`release`
   *   where appropriate to ensure visibility between threads.
   * - The capacity passed to the constructor is the maximum number of
   *   elements the queue will hold; internally the implementation allocates
   *   `capacity + 1` to distinguish full vs empty states.
   *
   * Typical pattern (producer/consumer):
   * - Producer thread calls `push_back()` or `push_front()` to insert items.
   * - Consumer thread calls `pop_front()`/`pop_back()` to remove items.
   * - Use `empty()`/`size()` to query state from the consumer side.
   *
   * The class also exposes an STL-like `iterator` for convenient iteration.
   */
  template <std::copyable T> requires std::equality_comparable<T>
  class CircularQueue {
    public:

      /**
       * @brief Forward iterator over the occupied elements of the queue.
       *
       * The iterator stores pointers into the internal buffer and wraps from
       * the end of the allocated storage back to the front. It is intentionally
       * lightweight (no reference to the parent queue) and meant for single-
       * threaded iteration by the consumer.
       */
      class iterator {
        public:
          /**
           * @brief Construct an iterator at a given pointer.
           * @param pointer Pointer to the current element.
           * @param front Pointer to the first element in the buffer storage.
           * @param end Pointer one-past-the-last element in the buffer storage.
           */
          iterator(T* pointer, T* front, T* end) : pointer_(pointer), front_(front), end_(end) { }

          /**
           * @brief Advance the iterator by one element, wrapping to `front_`
           *        when `end_` is reached.
           */
          inline void increment() {
            if (pointer_ == end_)
              pointer_ = front_;
            else
              pointer_++;
          }

          /**
           * @brief Pre-increment. Returns iterator value before increment to
           *        match the previous implementation semantics.
           */
          iterator operator++() {
            iterator iter = *this;
            increment();
            return iter;
          }

          /**
           * @brief Post-increment (conventional signature with `int`).
           */
          iterator operator++(int i) {
            iterator iter = *this;
            increment();
            return iter;
          }

          /**
           * @brief Dereference the iterator.
           *
           * The returned reference is valid as long as the element in the
           * queue is not overwritten by the producer; consumers should copy
           * out the value quickly if they need to retain it.
           */
          T& operator*() {
            return *pointer_;
          }

          /**
           * @brief Pointer-like access to the element.
           */
          T* operator->() {
            return pointer_;
          }

          /**
           * @brief Return the raw pointer to the current element.
           */
          T* get() {
            return pointer_;
          }

          /**
           * @brief Three-way comparison provided for iterator comparisons.
           */
          auto operator<=>(const iterator& rhs) const = default;

        protected:
          T* pointer_;
          T* front_;
          T* end_;
      };

      /**
       * @brief Construct a CircularQueue with a given maximum capacity.
       * @param capacity Maximum number of elements that can be stored.
       *
       * The implementation allocates `capacity + 1` storage internally to
       * distinguish the full vs. empty condition. After construction no
       * further allocations happen until `reserve()` is called.
       */
      CircularQueue(int capacity) : capacity_(capacity + 1), start_(0), end_(0) {
        data_ = new T[capacity_];
      }

      /**
       * @brief Default constructor creates an empty queue (no storage).
       * Call `reserve()` before use to allocate storage.
       */
      CircularQueue() : data_(nullptr), capacity_(0), start_(0), end_(0) { }

      /**
       * @brief Allocate (or re-allocate) internal storage for the queue.
       * @param capcity Maximum number of elements to reserve.
       *
       * This resets the queue content and allocates a new internal array of
       * size `capcity + 1`. Any previous contents are discarded. This method
       * may be called from non-real-time threads; avoid calling it from the
       * audio thread while the queue is in use.
       */
      void reserve(int capcity) {
        // Free previously allocated storage. Use delete[] because `data_`
        // points to an array allocated with new T[]. Using plain delete
        // is undefined behaviour.
        delete[] data_;

        capacity_ = capcity + 1;
        data_ = new T[capacity_];
        // Reset the indices to an empty state. We use release ordering to
        // ensure any prior modifications to `data_` are visible to other
        // threads that perform an acquire load on `start_`/`end_`.
        start_.store(0, std::memory_order_release);
        end_.store(0, std::memory_order_release);
      }

      /**
       * @brief Random access by logical index (0 == front element).
       * @param index Logical index from the front (0-based).
       * @return Reference to the element at that logical index.
       *
       * This performs an atomic load of the start index, computes the
       * physical offset in the backing array, and returns a reference. The
       * returned reference may be invalidated by concurrent producer writes.
       */
      T& operator[](std::size_t index) {
        int current_start = start_.load(std::memory_order_acquire);
        return data_[(current_start + static_cast<int>(index)) % capacity_];
      }

      /**
       * @brief Const overload of operator[].
       */
      const T& operator[](std::size_t index) const {
        int current_start = start_.load(std::memory_order_acquire);
        return data_[(current_start + static_cast<int>(index)) % capacity_];
      }

      /**
       * @brief Push an element to the back (tail) of the queue.
       * @param entry Value to insert (copied into the buffer).
       *
       * This operation writes the element at the current end index and
       * advances the `end_` atomic. In the single-producer model this is
       * safe without locks. Note: this implementation does not check for the
       * full condition; callers should avoid overflowing the buffer.
       */
      /**
       * @note Thread-safety: intended for the single-producer thread.
       * The consumer must not call `push_back()` concurrently.
       *
       * Memory ordering: we load `end_` with acquire semantics and store the
       * updated value with release semantics to ensure correct ordering of
       * writes to `data_` with respect to the index update.
       */
      void push_back(T entry) {
        int current_end = end_.load(std::memory_order_acquire);
        data_[current_end] = entry;
        end_.store((current_end + 1) % capacity_, std::memory_order_release);
      }

      /**
       * @brief Pop an element from the back (tail) of the queue and return it.
       * @return The element previously at the back.
       *
       * The consumer decrements `end_` and returns the stored value. If used
       * in a single-consumer scenario this is safe; no underflow checks are
       * provided here (caller must ensure the queue is not empty).
       */
      /**
       * @note Thread-safety: intended for the single-consumer thread.
       * The producer must not call `pop_back()` concurrently.
       *
       * This operation decrements the `end_` index with release semantics
       * after reading it with acquire semantics. The returned value is the
       * element previously stored at the back. Caller must ensure the queue
       * is not empty (use `empty()` or `size()` to check).
       */
      /**
       * @brief Pop an element from the back (tail) of the queue and return it.
       * @return The element previously at the back.
       */
      T pop_back() {
        int current_end = end_.load(std::memory_order_acquire);
        int new_end = (current_end - 1 + capacity_) % capacity_;
        end_.store(new_end, std::memory_order_release);
        return data_[new_end];
      }

      /**
       * @brief Push an element to the front (head) of the queue.
       * @param entry Value to insert (copied into the buffer).
       *
       * This moves the `start_` index backward and writes the element. As
       * with `push_back()`, no overflow detection is performed here.
       */
      /**
       * @note Thread-safety: intended for the single-producer thread.
       * Moves the `start_` index backward and writes the entry at the new
       * start position. No overflow detection is performed.
       */
      void push_front(T entry) {
        int current_start = start_.load(std::memory_order_acquire);
        int new_start = (current_start - 1 + capacity_) % capacity_;
        data_[new_start] = entry;
        start_.store(new_start, std::memory_order_release);
      }

      /**
       * @brief Pop an element from the front (head) of the queue and return it.
       * @return The element previously at the front.
       */
      /**
       * @note Thread-safety: intended for the single-consumer thread.
       * Reads the element at the current `start_` and advances the index.
       */
      /**
       * @brief Pop an element from the front (head) of the queue and return it.
       * @return The element previously at the front.
       */
      T pop_front() {
        int current_start = start_.load(std::memory_order_acquire);
        T result = data_[current_start];
        start_.store((current_start + 1) % capacity_, std::memory_order_release);
        return result;
      }

      /**
       * @brief Remove element at logical `index` (0 == front) by shifting
       *        subsequent elements towards the front.
       * @param index Logical index to remove.
       *
       * This operation has O(n) cost in the number of elements shifted, but
       * is useful for occasional removal of specific events. It recomputes
       * physical indices using the current atomic `start_` and `end_` values.
       */
      void removeAt(int index) {
        int current_start = start_.load(std::memory_order_acquire);
        int current_end = end_.load(std::memory_order_acquire);
        int i = (index + current_start) % capacity_;
        int new_end = (current_end - 1 + capacity_) % capacity_;
        while (i != new_end) {
          int next = (i + 1) % capacity_;
          data_[i] = data_[next];
          i = next;
        }
        end_.store(new_end, std::memory_order_release);
      }

      /**
       * @brief Remove the first occurrence of `entry` from the queue.
       * @param entry Element to remove (compared using operator==).
       */
      void remove(T entry) {
        int current_start = start_.load(std::memory_order_acquire);
        int current_end = end_.load(std::memory_order_acquire);
        for (int i = current_start; i != current_end; i = (i + 1) % capacity_) {
          if (data_[i] == entry) {
            removeAt((i - current_start + capacity_) % capacity_);
            return;
          }
        }
      }

      /**
       * @brief Remove all occurrences of `entry` from the queue.
       *
       * The implementation iterates through current contents and removes
       * matching elements. Note that `i--` is used to correct the loop when
       * the buffer contents shift due to a removal.
       */
      void removeAll(T entry) {
        int current_start = start_.load(std::memory_order_acquire);
        int current_end = end_.load(std::memory_order_acquire);
        for (int i = current_start; i != current_end; i = (i + 1) % capacity_) {
          if (data_[i] == entry) {
            removeAt((i - current_start + capacity_) % capacity_);
            i--;
            current_end = end_.load(std::memory_order_acquire);
          }
        }
      }

      /**
       * @brief Erase the element pointed to by `iter` and return the iterator
       *        to the next element (semantics preserved by caller).
       *
       * This computes the logical index from the raw pointer and calls
       * `removeAt()` to shift elements.
       */
      iterator erase(iterator& iter) {
        int index = iter.get() - data_;
        removeAt((index - start_ + capacity_) % capacity_);
        return iter;
      }

      /**
       * @brief Count occurrences of `entry` in the queue.
       * @return Number of matching elements.
       */
      int count(T entry) const {
        int number = 0;
        for (int i = start_; i != end_; i = (i + 1) % capacity_) {
          if (data_[i] == entry)
            number++;
        }
        return number;
      }

      /**
       * @brief Clear the queue content.
       *
       * Resets start and end indices to the empty state. This operation is
       * atomic and safe to call from either thread, but will discard any
       * unprocessed elements.
       */
      void clear() {
        start_.store(0, std::memory_order_release);
        end_.store(0, std::memory_order_release);
      }

      /**
       * @brief Return the front (oldest) element.
       *
       * Caller must ensure the queue is not empty before calling this.
       */
      T front() const {
        int current_start = start_.load(std::memory_order_acquire);
        return data_[current_start];
      }

      /**
       * @brief Return the back (newest) element.
       *
       * Caller must ensure the queue is not empty before calling this.
       */
      T back() const {
        int current_end = end_.load(std::memory_order_acquire);
        return data_[(current_end - 1 + capacity_) % capacity_];
      }

      /**
       * @brief Return the current number of elements in the queue.
       */
      int size() const {
        int current_start = start_.load(std::memory_order_acquire);
        int current_end = end_.load(std::memory_order_acquire);
        return (current_end - current_start + capacity_) % capacity_;
      }

      /**
       * @brief Return true if the queue is empty.
       */
      bool empty() const {
        return start_.load(std::memory_order_acquire) == end_.load(std::memory_order_acquire);
      }

      /**
       * @brief STL-style begin iterator to the front element.
       *
       * Note: the iterator is valid for iteration until the producer wraps
       * around and overwrites buffer elements. Use with care in concurrent
       * contexts (prefer consuming elements quickly).
       */
      iterator begin() const {
        int current_start = start_.load(std::memory_order_acquire);
        return iterator(data_ + current_start, data_, data_ + (capacity_ - 1));
      }

      /**
       * @brief STL-style end iterator (one-past-last occupied element).
       */
      iterator end() const {
        int current_end = end_.load(std::memory_order_acquire);
        return iterator(data_ + current_end, data_, data_ + (capacity_ - 1));
      }

    private:
      // Raw pointer to the allocated storage block. The memory layout is a
      // circular buffer of size `capacity_` (where capacity_ == requested
      // capacity + 1). Elements are stored contiguously for cache efficiency.
      T* data_;

      // Allocated capacity (internal size = user_capacity + 1).
      int capacity_;

      // Atomic indices indicating the logical start (read index) and end
      // (write index) into `data_`. These are manipulated with
      // acquire/release ordering to ensure correct visibility between the
      // producer and consumer without locks.
      std::atomic<int> start_;
      std::atomic<int> end_;
  };
} // namespace mopo

#endif // CIRCULAR_QUEUE_H

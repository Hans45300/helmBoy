#pragma once

#include "concurrentqueue.h"
#include "value.h"

#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <memory>
#include <new>

namespace helmboy {
  /** Existing control transport with prepared host producers and per-control overflow.
      UI enqueue may allocate as before. Host enqueue never creates producers or blocks.
      One consumer applies controls. Slots and controls must outlive queued changes.
      Host values originate from ValueBridge's float conversion; overflow stores those
      exact float bits. Sequence comparisons allow fewer than 2^30 outstanding changes
      per control. Intermediate host values can coalesce only on overflow/contention. */
  class ParameterChangeQueue {
    public:
      static constexpr std::size_t kPreparedProducers = 8;
      static constexpr std::size_t kInitialCapacity = 4096;
      static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
      static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
      static_assert(std::atomic<mopo::mopo_float>::is_always_lock_free);
      static_assert(std::atomic<bool>::is_always_lock_free);

      struct Slot {
        mopo::Value* control = nullptr;
        std::atomic<std::uint32_t> sequence { 0 };
        std::atomic<std::uint64_t> overflow { 0 };
        std::uint32_t applied = 0;
      };

      struct Change {
        Slot* slot = nullptr;
        mopo::mopo_float value = 0.0;
        std::uint32_t sequence = 0;
      };

      ParameterChangeQueue() : queue_(kInitialCapacity) {
        for (auto& producer : producers_) {
          producer = std::make_unique<Producer>(queue_);
          if (!producer->token.valid())
            throw std::bad_alloc();
        }
      }

      /** Non-audio UI path: preserve native value precision and allocating enqueue. */
      bool enqueue(Slot& slot, mopo::mopo_float value) {
        return queue_.enqueue(Change { &slot, value, nextSequence(slot) });
      }

      /** Multi-producer host path. False means retained in the overflow mailbox,
          not discarded. No token is shared concurrently; busy tokens are skipped. */
      bool enqueueHost(Slot& slot, float value) {
        const Change change { &slot, value, nextSequence(slot) };
        for (auto& producer : producers_) {
          if (producer->busy.test_and_set(std::memory_order_acquire))
            continue;
          const bool queued = queue_.try_enqueue(producer->token, change);
          producer->busy.clear(std::memory_order_release);
          if (queued)
            return true;
        }
         const std::uint64_t packed = kPendingBit | (std::uint64_t(change.sequence) << 32) |
                                     std::bit_cast<std::uint32_t>(value);
        auto previous = slot.overflow.load(std::memory_order_acquire);
         while ((previous & kPendingBit) == 0 ||
           isNewer(change.sequence, static_cast<std::uint32_t>(previous >> 32) & kSequenceMask)) {
          if (slot.overflow.compare_exchange_weak(previous, packed,
                  std::memory_order_acq_rel, std::memory_order_acquire))
            break;
        }
        overflow_pending_.store(true, std::memory_order_release);
        return false;
      }

      bool tryDequeue(Change& change) { return queue_.try_dequeue(change); }
      bool takeOverflowPending() {
        return overflow_pending_.load(std::memory_order_acquire) &&
               overflow_pending_.exchange(false, std::memory_order_acq_rel);
      }

      /** Single-consumer application prevents a late older packet reverting a value. */
      static bool apply(const Change& change) {
        if (change.slot == nullptr || change.slot->control == nullptr ||
            !isNewer(change.sequence, change.slot->applied))
          return false;
        change.slot->control->set(change.value);
        change.slot->applied = change.sequence;
        return true;
      }

      static bool applyOverflow(Slot& slot) {
        const auto packed = slot.overflow.fetch_and(~kPendingBit, std::memory_order_acq_rel);
        if ((packed & kPendingBit) == 0)
          return false;
        return apply(Change { &slot,
            std::bit_cast<float>(static_cast<std::uint32_t>(packed)),
            static_cast<std::uint32_t>(packed >> 32) & kSequenceMask });
      }

    private:
      static constexpr std::uint64_t kPendingBit = std::uint64_t { 1 } << 63;
      static constexpr std::uint32_t kSequenceMask = 0x7fffffffu;
      using Queue = moodycamel::ConcurrentQueue<Change>;
      struct Producer {
        explicit Producer(Queue& queue) : token(queue) { }
        moodycamel::ProducerToken token;
        std::atomic_flag busy = ATOMIC_FLAG_INIT;
      };

      static std::uint32_t nextSequence(Slot& slot) {
        return (slot.sequence.fetch_add(1, std::memory_order_relaxed) + 1) & kSequenceMask;
      }

      static bool isNewer(std::uint32_t candidate, std::uint32_t previous) {
        const std::uint32_t distance = (candidate - previous) & kSequenceMask;
        return distance != 0 && distance < 0x40000000u;
      }

      Queue queue_;
      std::array<std::unique_ptr<Producer>, kPreparedProducers> producers_;
      std::atomic<bool> overflow_pending_ { false };
  };
}
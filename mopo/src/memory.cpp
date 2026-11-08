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

#include "memory.h"

/**
 * @file memory.cpp
 * @brief Memory utilities used by mopo (small abstractions over new/delete).
 *
 * This file centralizes any helpers dealing with raw allocation so that
 * platform-specific adjustments or leak detection are easier to apply.
 */
#include "utils.h"

#include <cmath>
#include <cstring>

namespace mopo {

  Memory::Memory(int size) : offset_(0), valid_samples_(0) {
    size_ = utils::nextPowerOfTwo(size);
    bitmask_ = size_ - 1;
    memory_ = new mopo_float[size_];
    utils::zeroBuffer(std::span<mopo_float>(memory_, size_));
  }

  Memory::Memory(const Memory& other) : valid_samples_(0) {
    this->memory_ = new mopo_float[other.size_];
    utils::zeroBuffer(std::span<mopo_float>(this->memory_, other.size_));
    this->size_ = other.size_;
    this->bitmask_ = other.bitmask_;
    std::atomic_ref<const unsigned int> other_atomic_offset(other.offset_);
    this->offset_ = other_atomic_offset.load(std::memory_order_acquire);
  }

  Memory::~Memory() {
    delete[] memory_;
  }
} // namespace mopo

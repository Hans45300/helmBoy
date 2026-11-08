#pragma once

#include "common.h"

#include <array>
#include <cstddef>

namespace helmboy {

class ParameterEditHistory {
  public:
    struct Step {
      std::size_t parameter_index = 0;
      mopo::mopo_float start_value = 0.0;
      mopo::mopo_float end_value = 0.0;
    };

    static constexpr std::size_t kCapacity = 256;
    static_assert(sizeof(Step) * kCapacity <= 6 * 1024,
            "Undo history ring must remain within 6 KiB");

    bool append(std::size_t parameter_index,
                mopo::mopo_float start_value,
                mopo::mopo_float end_value) {
      if (start_value == end_value)
        return false;

      if (cursor_ < size_)
        size_ = cursor_;

      if (size_ == kCapacity) {
        begin_ = (begin_ + 1) % kCapacity;
        --size_;
        --cursor_;
      }

      steps_[(begin_ + size_) % kCapacity] =
          { parameter_index, start_value, end_value };
      ++size_;
      cursor_ = size_;
      return true;
    }

    bool undo(Step& step) {
      if (cursor_ == 0)
        return false;

      --cursor_;
      step = steps_[(begin_ + cursor_) % kCapacity];
      return true;
    }

    bool redo(Step& step) {
      if (cursor_ >= size_)
        return false;

      step = steps_[(begin_ + cursor_) % kCapacity];
      ++cursor_;
      return true;
    }

    void clear() {
      begin_ = 0;
      size_ = 0;
      cursor_ = 0;
    }

    [[nodiscard]] bool canUndo() const { return cursor_ > 0; }
    [[nodiscard]] bool canRedo() const { return cursor_ < size_; }
    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] std::size_t cursor() const { return cursor_; }

  private:
    std::array<Step, kCapacity> steps_ {};
    std::size_t begin_ = 0;
    std::size_t size_ = 0;
    std::size_t cursor_ = 0;
};

} // namespace helmboy
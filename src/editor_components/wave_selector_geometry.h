#pragma once

#include <algorithm>

namespace helmBoy::ui {

struct WaveSelectorCellBounds {
  float x;
  float y;
  float width;
  float height;
};

inline int getWaveSelectorTypesPerLine(int minimum, int maximum) {
  const int count = maximum - minimum + 1;
  return count > 0 ? (count + 1) / 2 : 0;
}

inline WaveSelectorCellBounds getWaveSelectorCellBounds(
    int waveform_index, int minimum, int maximum, float width, float height,
    float row_padding) {
  const int count = maximum - minimum + 1;
  const int types_per_line = getWaveSelectorTypesPerLine(minimum, maximum);
  if (count <= 0 || types_per_line <= 0)
    return {};

  const int relative_index = std::clamp(waveform_index - minimum, 0, count - 1);
  const int row = relative_index / types_per_line;
  const int column = relative_index % types_per_line;
  const float cell_height = height / 2.0f;
  return { column * width / types_per_line,
           row * cell_height - (row > 0 ? row_padding : 0.0f),
           width / types_per_line,
           cell_height };
}

} // namespace helmBoy::ui
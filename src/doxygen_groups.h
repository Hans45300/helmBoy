/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * helmBoy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file doxygen_groups.h
 * @brief Doxygen group definitions for documentation organization
 */

#ifndef DOXYGEN_GROUPS_H
#define DOXYGEN_GROUPS_H

/**
 * @defgroup synthesis Synthesis Engine
 * @brief Core audio synthesis and DSP components
 *
 * Contains all classes and modules responsible for sound generation,
 * signal processing, modulation, and audio rendering.
 */

/**
 * @defgroup user_interface User Interface
 * @brief GUI components and visual elements
 *
 * Contains all UI components including controls, displays, editors,
 * and visual feedback elements for the synthesizer interface.
 */

/**
 * @defgroup plugin Plugin Interface
 * @brief Plugin wrapper and host integration
 *
 * Contains plugin-specific code for VST3, LV2, and standalone builds,
 * including parameter bridging and host communication.
 */

/**
 * @defgroup midi MIDI Processing
 * @brief MIDI input/output and message handling
 *
 * Contains the active MIDI 1.0 input path and the explicit, limited UMP
 * decoder. Profile/property output helpers are experimental and are not
 * connected to host input or voice processing.
 */

/**
 * @defgroup common Common Utilities
 * @brief Shared utilities and helper classes
 *
 * Contains common functionality used across the application including
 * configuration management, preset handling, and utility functions.
 */

#endif // DOXYGEN_GROUPS_H

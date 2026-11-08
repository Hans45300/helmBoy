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
/**
 * @file tick_router.h
 * @brief Routage et traitement échantillon-par-échantillon (TickRouter)
 */
#ifndef TICK_ROUTER_H
#define TICK_ROUTER_H

#include "processor_router.h"

namespace mopo {

  /**
   * @brief Specialized processor router for sample-by-sample processing
   * @ingroup mopo_modules
   *
   * TickRouter extends ProcessorRouter to provide sample-by-sample processing
   * capabilities. This is essential for modules that need per-sample control
   * or processing, such as envelopes, LFOs, and real-time parameter smoothing.
   *
   * @section tick_processing Tick-based Processing
   *
   * The tick processing model offers:
   * - **Sample-accurate Processing**: Individual sample control and timing
   * - **Parameter Interpolation**: Smooth parameter changes within audio blocks
   * - **Control Rate Processing**: Efficient processing of slow-changing signals
   * - **Event Handling**: Sample-accurate MIDI and automation events
   * - **Modulation Processing**: Real-time modulation source processing
   *
   * @section tick_architecture Architecture
   *
   * TickRouter provides a framework for:
   * - Block-based audio processing with per-sample granularity
   * - Efficient routing of control and audio signals
   * - Sample-accurate parameter updates
   * - Modulation source synchronization
   * - Event scheduling and processing
   *
  * @section tick_implementation Implementation Pattern
   *
   * Typical implementation follows this pattern:
   * ```cpp
   * class MyTickProcessor : public TickRouter {
   * public:
   *   void process() override {
   *     for (int i = 0; i < buffer_size; ++i) {
   *       tick(i);  // Process individual sample
   *     }
   *   }
   *
   *   void tick(int i) override {
   *     // Per-sample processing logic
   *     mopo_float sample = processSample();
   *     output(0)->buffer[i] = sample;
   *   }
   * };
   * ```
   *
  * @section tick_performance Performance Considerations
   *
   * While tick-based processing provides sample accuracy, consider:
   * - Use block processing for heavy computations when possible
   * - Implement control rate processing for slow parameters
   * - Cache expensive calculations outside the tick loop
   * - Use SIMD operations for vector processing when appropriate
   *
   * @see ProcessorRouter
   * @see Processor
   * @see HelmBoyModule
   */
  /**
   * @brief Routeur de processeur sp�cialis� pour le traitement �chantillon par �chantillon.
   */
  class TickRouter : public ProcessorRouter {
    public:
      /**
       * @brief Constructeur.
       * @param num_inputs Nombre d'entr�es.
       * @param num_outputs Nombre de sorties.
       */
      TickRouter(int num_inputs = 0, int num_outputs = 0)
        : ProcessorRouter(num_inputs, num_outputs) {}

      /**
       * @brief Traite le bloc courant (doit �tre red�fini).
       */
      virtual void process() override = 0;

      /**
       * @brief Traite un �chantillon (doit �tre red�fini).
       * @param i Index de l'�chantillon.
       */
      virtual void tick(int i) = 0;
  };
} // namespace mopo

#endif // PROCESSOR_ROUTER_H

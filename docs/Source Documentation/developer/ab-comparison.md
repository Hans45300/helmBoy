# A/B comparison and Init

@page ab_comparison A/B comparison and Init

The Patch Selector owns two lazy parameter snapshots. The first A/B action
initializes both slots from the current synth state and activates B; each
subsequent action captures the active slot before switching to the other.
Snapshots contain all engine controls and active modulation routes, including
modulation depths, but do not contain patch metadata or runtime DSP state.

Loading a preset changes the live controls. The new state is captured into the
currently active slot the next time A/B is switched. Init replaces only the
active slot with the parameter defaults used by `LoadSave::initSynth` and
removes its modulation routes; the other slot is left intact.

Snapshot changes use `SynthBase`'s existing control and modulation queues.
Parameter values are queued together for application at the start of an audio
block and sent through the host-notification path. The message thread updates
the GUI from the requested snapshot. No DSP buffers or voice/effect state are
copied. This reuses existing block-boundary updates; it does not add dedicated
per-parameter ramping, so verify transitions during playback in plugin and
standalone hosts.

Manual verification:

1. Change several parameters and modulation routes in A, switch to B, and
   confirm each parameter and route returns to A's saved snapshot.
2. Edit B, switch repeatedly, and confirm neither slot loses its changes.
3. Load a preset while B is active; confirm A is unchanged and the loaded
   parameters become B's state on the next switch.
4. Invoke Init in either slot; confirm it resets all controls and routes in
   that slot while preserving the other slot.
5. In a plugin host, verify parameter values and automation lanes follow A/B
   and Init changes. Listen for clicks while switching during playback.

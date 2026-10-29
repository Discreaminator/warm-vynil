#pragma once

#include <atomic>

namespace wg::plugin {

/** Host transport → UI (SPEC §9.2). Written from audio thread, read from UI. */
struct TransportState
{
    std::atomic<bool> isPlaying{ false };
};

} // namespace wg::plugin

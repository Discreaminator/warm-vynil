#pragma once

namespace wg::dsp {

/** POD snapshot of user-facing parameters (filled by ParamBridge in later milestones). */
struct EngineParams
{
    float amount = 0.5f;
    float outGainDb = 0.0f;
    bool bypass = false;
};

} // namespace wg::dsp

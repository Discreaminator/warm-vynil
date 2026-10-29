#include "Engine.h"

namespace wg::dsp {

void Engine::prepare(const Spec& spec) noexcept
{
    spec_ = spec;
    latencySamples_ = 0;
    reset();
}

void Engine::reset() noexcept
{
    // No state in M0 passthrough.
}

void Engine::setTargets(const EngineParams& params) noexcept
{
    targets_ = params;
}

void Engine::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    // Passthrough: leave buffers unchanged. Ignore unused targets for M0.
    (void)channels;
    (void)numChannels;
    (void)numSamples;
    (void)targets_;
}

} // namespace wg::dsp

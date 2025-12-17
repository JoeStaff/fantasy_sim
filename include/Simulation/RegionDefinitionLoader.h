#pragma once

#include "Core/Config.h"

namespace Simulation {

// Load region definitions into the config
// TODO: Replace with proper JSON parsing when JSON library is integrated
void LoadRegionDefinitions(Config::RegionsConfig& regions_config);

} // namespace Simulation



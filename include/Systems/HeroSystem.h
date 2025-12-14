#pragma once

#include "ECS/System.h"
#include "Core/Types.h"

namespace Systems {

// Hero system - manages hero updates and influences
class HeroSystem : public ECS::System {
public:
    HeroSystem();
    ~HeroSystem() override = default;
    
    void Update(f32 delta_time) override;
    
    // Update hero influences
    void UpdateHeroInfluences();
    
    // Process hero events
    void ProcessHeroEvents();
};

} // namespace Systems

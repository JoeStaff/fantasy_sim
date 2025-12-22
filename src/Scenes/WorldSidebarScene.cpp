#include "Scenes/WorldSidebarScene.h"
#include "Scenes/WorldScene.h"
#include "Simulation/Region.h"
#include "Simulation/World.h"
#include "Platform/IVideo.h"
#include "Platform/IInput.h"
#include <iostream>

namespace Game {

// Shared state definitions
namespace WorldSceneSharedState {
    RegionID g_selected_region_id = INVALID_REGION_ID;
    Simulation::SimulationManager* g_simulation_manager = nullptr;
}

WorldSidebarScene::WorldSidebarScene()
    : Scene("WorldSidebarScene")
{
}

bool WorldSidebarScene::Initialize(Platform::PlatformManager* platform_manager) {
    platform_manager_ = platform_manager;
    return true;
}

void WorldSidebarScene::Shutdown() {
    // Nothing to cleanup
}

void WorldSidebarScene::Update(f32 delta_time) {
    // Sidebar doesn't need updates
    (void)delta_time;
}

void WorldSidebarScene::Render(Platform::IVideo* video) {
    if (!video) {
        return;
    }
    
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    
    // Draw sidebar background
    video->SetDrawColor(40, 40, 50, 255);
    video->DrawRect(0, 0, window_width, window_height);
    
    // Draw sidebar border
    video->SetDrawColor(60, 60, 70, 255);
    video->DrawLine(0, 0, 0, window_height);
    video->DrawLine(window_width - 1, 0, window_width - 1, window_height);
    
    // Render region stats if a region is selected
    if (WorldSceneSharedState::g_selected_region_id != INVALID_REGION_ID && 
        WorldSceneSharedState::g_simulation_manager) {
        const Simulation::Region* region = 
            WorldSceneSharedState::g_simulation_manager->GetRegion(WorldSceneSharedState::g_selected_region_id);
        if (region) {
            RenderRegionStats(video, region);
        }
    } else {
        // No region selected - show placeholder text
        video->SetDrawColor(200, 200, 200, 255);
        video->DrawText("No region selected", 10, 20, 200, 200, 200, 255);
        video->DrawText("Click a region to view stats", 10, 50, 150, 150, 150, 255);
    }
}

void WorldSidebarScene::ProcessInput(Platform::IInput* input) {
    // Sidebar doesn't process input (it's display-only)
    (void)input;
}

void WorldSidebarScene::OnEnter() {
    // Nothing to do on enter
}

void WorldSidebarScene::OnExit() {
    // Nothing to do on exit
}

void WorldSidebarScene::RenderRegionStats(Platform::IVideo* video, const Simulation::Region* region) {
    if (!video || !region) {
        return;
    }
    
    i32 y_pos = 20;
    const i32 line_height = 25;
    const i32 title_size = 18;
    const i32 text_size = 14;
    
    // Title
    video->SetFontSize(title_size);
    video->SetDrawColor(255, 255, 255, 255);
    video->DrawText("Region Stats", 10, y_pos, 255, 255, 255, 255);
    y_pos += line_height + 10;
    
    // Draw separator line
    video->SetDrawColor(100, 100, 100, 255);
    i32 window_width = video->GetWindowWidth();
    video->DrawLine(10, y_pos, window_width - 10, y_pos);
    y_pos += 15;
    
    video->SetFontSize(text_size);
    
    // Region Name (if it's a source region)
    const std::string& name = region->GetName();
    if (!name.empty()) {
        video->SetDrawColor(255, 255, 200, 255);  // Highlight name
        video->DrawText("Name: " + name, 10, y_pos, 255, 255, 200, 255);
        y_pos += line_height;
    }
    
    // Settlement Role (if it's a settlement)
    if (WorldSceneSharedState::g_simulation_manager) {
        const Simulation::World* world = WorldSceneSharedState::g_simulation_manager->GetWorld();
        if (world) {
            const auto& settlements = world->GetSettlements();
            for (const auto& settlement : settlements) {
                if (settlement.region_id == region->GetID()) {
                    std::string role = settlement.type;
                    // Determine role description based on context
                    if (settlement.type == "City") {
                        // Check neighbors to determine role
                        bool near_mountain = false;
                        bool near_water = false;
                        bool near_forest = false;
                        
                        u16 grid_x = settlement.grid_x;
                        u16 grid_y = settlement.grid_y;
                        
                        if (world) {
                            const Simulation::Region* neighbors[4] = {
                                world->GetRegionAtGrid(grid_x, grid_y - 1),
                                world->GetRegionAtGrid(grid_x, grid_y + 1),
                                world->GetRegionAtGrid(grid_x - 1, grid_y),
                                world->GetRegionAtGrid(grid_x + 1, grid_y)
                            };
                            
                            for (const Simulation::Region* neighbor : neighbors) {
                                if (neighbor) {
                                    if (neighbor->GetType() == "Mountain") near_mountain = true;
                                    if (neighbor->GetType() == "Coastal" || neighbor->GetType() == "River") near_water = true;
                                    if (neighbor->GetType() == "Forest") near_forest = true;
                                }
                            }
                        }
                        
                        if (near_mountain) role = "Mountain Settlement";
                        else if (near_water) role = "Coastal Settlement";
                        else if (near_forest) role = "Forest Settlement";
                        else role = "Plains Settlement";
                    } else if (settlement.type == "Village") {
                        role = "Village";
                    } else if (settlement.type == "Capital") {
                        role = "Capital";
                    }
                    
                    video->SetDrawColor(200, 255, 200, 255);  // Highlight settlement role
                    video->DrawText("Role: " + role, 10, y_pos, 200, 255, 200, 255);
                    y_pos += line_height;
                    break;
                }
            }
        }
    }
    
    // Region ID
    video->SetDrawColor(200, 200, 200, 255);
    video->DrawText("ID: " + std::to_string(region->GetID()), 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Region Type
    std::string type = region->GetType();
    video->DrawText("Type: " + type, 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Region Subtype (if exists)
    const std::string& subtype = region->GetSubtype();
    if (!subtype.empty()) {
        video->DrawText("Subtype: " + subtype, 10, y_pos, 200, 200, 200, 255);
        y_pos += line_height;
    }
    
    // Source region info
    if (region->IsSource()) {
        video->SetDrawColor(180, 200, 255, 255);
        video->DrawText("Source Region", 10, y_pos, 180, 200, 255, 255);
        y_pos += line_height;
    } else if (region->GetSourceParentID() != INVALID_REGION_ID) {
        const Simulation::Region* parent = WorldSceneSharedState::g_simulation_manager ? 
            WorldSceneSharedState::g_simulation_manager->GetRegion(region->GetSourceParentID()) : nullptr;
        if (parent) {
            video->SetDrawColor(180, 200, 255, 255);
            std::string parent_name = parent->GetName();
            if (parent_name.empty()) {
                parent_name = parent->GetType();
            }
            video->DrawText("Part of: " + parent_name, 10, y_pos, 180, 200, 255, 255);
            y_pos += line_height;
        }
    }
    
    y_pos += 5;  // Small gap
    
    // Population
    u32 population = region->GetPopulation();
    u32 capacity = region->GetCapacity();
    video->SetDrawColor(200, 200, 200, 255);
    video->DrawText("Population: " + std::to_string(population), 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Capacity
    video->DrawText("Capacity: " + std::to_string(capacity), 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Population percentage
    f32 pop_percent = capacity > 0 ? (static_cast<f32>(population) / static_cast<f32>(capacity)) * 100.0f : 0.0f;
    std::string pop_str = "Fullness: " + std::to_string(static_cast<int>(pop_percent)) + "%";
    video->DrawText(pop_str, 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height + 10;
    
    // Position
    video->DrawText("Position:", 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    std::string pos_str = "  X: " + std::to_string(static_cast<int>(region->GetX()));
    video->DrawText(pos_str, 10, y_pos, 180, 180, 180, 255);
    y_pos += line_height;
    pos_str = "  Y: " + std::to_string(static_cast<int>(region->GetY()));
    video->DrawText(pos_str, 10, y_pos, 180, 180, 180, 255);
    y_pos += line_height + 10;
    
    // Neighbors
    const auto& neighbors = region->GetNeighbors();
    video->DrawText("Neighbors: " + std::to_string(neighbors.size()), 10, y_pos, 200, 200, 200, 255);
}

} // namespace Game


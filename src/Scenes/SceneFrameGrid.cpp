#include "Scenes/SceneFrameGrid.h"
#include "Scenes/SceneFrame.h"
#include <algorithm>

namespace Game {

SceneFrameGrid::SceneFrameGrid(i32 grid_cols, i32 grid_rows)
    : grid_cols_(grid_cols > 0 ? grid_cols : 1)
    , grid_rows_(grid_rows > 0 ? grid_rows : 1)
{
}

bool SceneFrameGrid::AddScene(const std::string& scene_name, i32 grid_x, i32 grid_y, 
                              i32 grid_width, i32 grid_height) {
    // Validate grid bounds
    if (grid_x < 0 || grid_y < 0 || 
        grid_x + grid_width > grid_cols_ || 
        grid_y + grid_height > grid_rows_) {
        return false;
    }
    
    // Check for overlap (excluding this scene if it already exists)
    if (HasOverlap(grid_x, grid_y, grid_width, grid_height, scene_name)) {
        return false;
    }
    
    // Remove existing assignment for this scene if it exists
    RemoveScene(scene_name);
    
    // Add new assignment
    GridCellAssignment assignment;
    assignment.scene_name = scene_name;
    assignment.grid_x = grid_x;
    assignment.grid_y = grid_y;
    assignment.grid_width = grid_width;
    assignment.grid_height = grid_height;
    
    assignments_.push_back(assignment);
    return true;
}

void SceneFrameGrid::RemoveScene(const std::string& scene_name) {
    assignments_.erase(
        std::remove_if(assignments_.begin(), assignments_.end(),
            [&scene_name](const GridCellAssignment& assignment) {
                return assignment.scene_name == scene_name;
            }),
        assignments_.end()
    );
}

void SceneFrameGrid::CalculateFrameBounds(const std::string& scene_name, i32 window_width, i32 window_height,
                                         i32& x, i32& y, i32& width, i32& height) const {
    const GridCellAssignment* assignment = FindAssignment(scene_name);
    if (!assignment) {
        x = y = width = height = 0;
        return;
    }
    
    // Calculate cell size
    f32 cell_width = static_cast<f32>(window_width) / static_cast<f32>(grid_cols_);
    f32 cell_height = static_cast<f32>(window_height) / static_cast<f32>(grid_rows_);
    
    // Calculate frame position and size
    x = static_cast<i32>(assignment->grid_x * cell_width);
    y = static_cast<i32>(assignment->grid_y * cell_height);
    width = static_cast<i32>(assignment->grid_width * cell_width);
    height = static_cast<i32>(assignment->grid_height * cell_height);
}

bool SceneFrameGrid::IsCellOccupied(i32 grid_x, i32 grid_y) const {
    for (const auto& assignment : assignments_) {
        if (grid_x >= assignment.grid_x && 
            grid_x < assignment.grid_x + assignment.grid_width &&
            grid_y >= assignment.grid_y && 
            grid_y < assignment.grid_y + assignment.grid_height) {
            return true;
        }
    }
    return false;
}

void SceneFrameGrid::UpdateFrameBounds(std::unordered_map<std::string, SceneFrame*>& frames,
                                      i32 window_width, i32 window_height) const {
    for (const auto& assignment : assignments_) {
        auto it = frames.find(assignment.scene_name);
        if (it != frames.end() && it->second) {
            i32 x, y, width, height;
            CalculateFrameBounds(assignment.scene_name, window_width, window_height, x, y, width, height);
            it->second->SetBounds(x, y, width, height);
        }
    }
}

const GridCellAssignment* SceneFrameGrid::FindAssignment(const std::string& scene_name) const {
    for (const auto& assignment : assignments_) {
        if (assignment.scene_name == scene_name) {
            return &assignment;
        }
    }
    return nullptr;
}

bool SceneFrameGrid::HasOverlap(i32 grid_x, i32 grid_y, i32 grid_width, i32 grid_height,
                                const std::string& exclude_scene) const {
    for (const auto& assignment : assignments_) {
        // Skip the excluded scene
        if (!exclude_scene.empty() && assignment.scene_name == exclude_scene) {
            continue;
        }
        
        // Check for overlap
        if (!(grid_x + grid_width <= assignment.grid_x || 
              grid_x >= assignment.grid_x + assignment.grid_width ||
              grid_y + grid_height <= assignment.grid_y || 
              grid_y >= assignment.grid_y + assignment.grid_height)) {
            return true;
        }
    }
    return false;
}

} // namespace Game


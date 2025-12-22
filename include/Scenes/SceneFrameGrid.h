#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Game {

// Forward declaration
class SceneFrame;

// Represents a grid cell assignment for a scene
struct GridCellAssignment {
    std::string scene_name;
    i32 grid_x = 0;      // Grid column
    i32 grid_y = 0;      // Grid row
    i32 grid_width = 1;  // Number of columns to span
    i32 grid_height = 1; // Number of rows to span
};

// Manages a grid-based layout for scene frames
class SceneFrameGrid {
public:
    SceneFrameGrid(i32 grid_cols, i32 grid_rows);
    
    // Get grid dimensions
    i32 GetGridCols() const { return grid_cols_; }
    i32 GetGridRows() const { return grid_rows_; }
    
    // Add a scene to the grid at specific cells
    // Returns true if the cells are available, false if they overlap with existing assignments
    bool AddScene(const std::string& scene_name, i32 grid_x, i32 grid_y, i32 grid_width, i32 grid_height);
    
    // Remove a scene from the grid
    void RemoveScene(const std::string& scene_name);
    
    // Get all cell assignments
    const std::vector<GridCellAssignment>& GetAssignments() const { return assignments_; }
    
    // Calculate frame bounds for a scene based on window size
    void CalculateFrameBounds(const std::string& scene_name, i32 window_width, i32 window_height,
                              i32& x, i32& y, i32& width, i32& height) const;
    
    // Check if a grid cell is occupied
    bool IsCellOccupied(i32 grid_x, i32 grid_y) const;
    
    // Update all frame bounds based on current window size
    void UpdateFrameBounds(std::unordered_map<std::string, SceneFrame*>& frames, 
                          i32 window_width, i32 window_height) const;
    
private:
    i32 grid_cols_ = 1;
    i32 grid_rows_ = 1;
    std::vector<GridCellAssignment> assignments_;
    
    // Find assignment for a scene
    const GridCellAssignment* FindAssignment(const std::string& scene_name) const;
    
    // Check if cells overlap with existing assignments
    bool HasOverlap(i32 grid_x, i32 grid_y, i32 grid_width, i32 grid_height, 
                   const std::string& exclude_scene = "") const;
};

} // namespace Game


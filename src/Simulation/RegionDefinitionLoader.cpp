#include "Simulation/RegionDefinition.h"
#include "Core/Config.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Simulation {

// Load region definitions from JSON file
void LoadRegionDefinitions(Config::RegionsConfig& regions_config) {
    // Get the path from config
    auto& config = Config::Configuration::GetInstance();
    std::string json_path = config.regions.region_data_path;
    
    // Try to open the JSON file
    std::ifstream file(json_path);
    if (!file.is_open()) {
        std::cerr << "RegionDefinitionLoader: Failed to open JSON file: " << json_path << std::endl;
        std::cerr << "RegionDefinitionLoader: Falling back to hardcoded definitions" << std::endl;
        // Fall back to hardcoded if file not found
        return;
    }
    
    try {
        nlohmann::json json_data;
        file >> json_data;
        file.close();
        
        // Parse regions array
        if (!json_data.contains("regions") || !json_data["regions"].is_array()) {
            std::cerr << "RegionDefinitionLoader: Invalid JSON structure - missing 'regions' array" << std::endl;
            return;
        }
        
        for (const auto& region_json : json_data["regions"]) {
            RegionDefinition def;
            
            // Required fields
            if (!region_json.contains("type") || !region_json["type"].is_string()) {
                std::cerr << "RegionDefinitionLoader: Skipping region - missing or invalid 'type'" << std::endl;
                continue;
            }
            def.type = region_json["type"].get<std::string>();
            
            // Optional fields with defaults
            def.spawn_weight = region_json.value("spawn_weight", 1.0f);
            def.expansion_weight = region_json.value("expansion_weight", 1.0f);
            
            // Color array [r, g, b]
            if (region_json.contains("color") && region_json["color"].is_array() && region_json["color"].size() >= 3) {
                def.color_r = static_cast<u8>(region_json["color"][0].get<int>());
                def.color_g = static_cast<u8>(region_json["color"][1].get<int>());
                def.color_b = static_cast<u8>(region_json["color"][2].get<int>());
            } else {
                // Default gray if color not specified
                def.color_r = def.color_g = def.color_b = 128;
            }
            
            // Potential names array
            if (region_json.contains("potential_names") && region_json["potential_names"].is_array()) {
                for (const auto& name : region_json["potential_names"]) {
                    if (name.is_string()) {
                        def.potential_names.push_back(name.get<std::string>());
                    }
                }
            }
            
            // Source and expansion counts
            def.min_source_count = region_json.value("min_source_count", 0u);
            def.max_source_count = region_json.value("max_source_count", 0u);
            def.min_expansion_size = region_json.value("min_expansion_size", 0u);
            def.max_expansion_size = region_json.value("max_expansion_size", 0u);
            
            // Influence stats object
            if (region_json.contains("influence_stats") && region_json["influence_stats"].is_object()) {
                for (const auto& [key, value] : region_json["influence_stats"].items()) {
                    if (value.is_number()) {
                        def.influence_stats[key] = value.get<f32>();
                    }
                }
            }
            
            // Capacity
            def.capacity = region_json.value("capacity", 10000u);
            
            // Resource types array
            if (region_json.contains("resource_types") && region_json["resource_types"].is_array()) {
                for (const auto& resource : region_json["resource_types"]) {
                    if (resource.is_string()) {
                        def.resource_types.push_back(resource.get<std::string>());
                    }
                }
            }
            
            // Compatible neighbors array
            if (region_json.contains("compatible_neighbors") && region_json["compatible_neighbors"].is_array()) {
                for (const auto& neighbor : region_json["compatible_neighbors"]) {
                    if (neighbor.is_string()) {
                        def.compatible_neighbors.push_back(neighbor.get<std::string>());
                    }
                }
            }
            
            // Incompatible neighbors array
            if (region_json.contains("incompatible_neighbors") && region_json["incompatible_neighbors"].is_array()) {
                for (const auto& neighbor : region_json["incompatible_neighbors"]) {
                    if (neighbor.is_string()) {
                        def.incompatible_neighbors.push_back(neighbor.get<std::string>());
                    }
                }
            }
            
            // Prevent overwrite flag
            def.prevent_overwrite = region_json.value("prevent_overwrite", false);
            
            // Store the definition
            regions_config.region_definitions[def.type] = def;
        }
        
        std::cout << "RegionDefinitionLoader: Loaded " << regions_config.region_definitions.size() 
                  << " region definitions from " << json_path << std::endl;
        
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "RegionDefinitionLoader: JSON parsing error: " << e.what() << std::endl;
        std::cerr << "RegionDefinitionLoader: Falling back to hardcoded definitions" << std::endl;
        file.close();
        return;
    } catch (const std::exception& e) {
        std::cerr << "RegionDefinitionLoader: Error loading JSON: " << e.what() << std::endl;
        std::cerr << "RegionDefinitionLoader: Falling back to hardcoded definitions" << std::endl;
        file.close();
        return;
    }
}

} // namespace Simulation




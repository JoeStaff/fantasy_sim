#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <unordered_map>

namespace Events {

using EventID = u64;

// Event types
enum class EventType : u8 {
    Global = 0,
    Regional = 1,
    Individual = 2
};

// Event priority
enum class EventPriority : u8 {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

// Base event structure
struct Event {
    EventID id = 0;
    EventType type = EventType::Individual;
    EventPriority priority = EventPriority::Normal;
    Tick scheduled_tick = 0;
    Tick execution_tick = 0;
    std::string event_name;
    
    // Event data (flexible)
    std::unordered_map<std::string, std::string> string_data;
    std::unordered_map<std::string, f32> float_data;
    std::unordered_map<std::string, u32> int_data;
    
    // Affected entities/regions
    std::vector<EntityID> affected_entities;
    std::vector<RegionID> affected_regions;
    
    // Cascading events
    std::vector<EventID> triggered_events;
    
    Event() = default;
    virtual ~Event() = default;
};

// Event system
class EventSystem {
public:
    EventSystem();
    ~EventSystem();
    
    // Initialize event system
    void Initialize();
    
    // Update event system
    void Update(f32 delta_time, Tick current_tick);
    
    // Schedule an event
    EventID ScheduleEvent(std::unique_ptr<Event> event, Tick delay_ticks = 0);
    
    // Schedule immediate event
    EventID ScheduleImmediateEvent(std::unique_ptr<Event> event);
    
    // Cancel an event
    bool CancelEvent(EventID event_id);
    
    // Get event
    Event* GetEvent(EventID event_id);
    const Event* GetEvent(EventID event_id) const;
    
    // Register event handler
    void RegisterHandler(const std::string& event_name, std::function<void(Event&)> handler);
    
    // Create event templates
    std::unique_ptr<Event> CreateGlobalEvent(const std::string& name);
    std::unique_ptr<Event> CreateRegionalEvent(const std::string& name, RegionID region_id);
    std::unique_ptr<Event> CreateIndividualEvent(const std::string& name, EntityID entity_id);
    
    // Get event history
    const std::vector<std::unique_ptr<Event>>& GetEventHistory() const;
    
    // Get active event count
    u32 GetActiveEventCount() const;
    
private:
    Config::EventsConfig config_;
    
    // Event storage
    std::priority_queue<std::unique_ptr<Event>, std::vector<std::unique_ptr<Event>>, EventComparator> event_queue_;
    std::unordered_map<EventID, Event*> active_events_;
    std::vector<std::unique_ptr<Event>> event_history_;
    
    // Event handlers
    std::unordered_map<std::string, std::function<void(Event&)>> event_handlers_;
    
    EventID next_event_id_ = 1;
    Tick current_tick_ = 0;
    
    void ProcessEventQueue(Tick current_tick);
    void ExecuteEvent(Event& event);
    void HandleCascadingEvents(Event& event);
    
    // Event comparator for priority queue
    struct EventComparator {
        bool operator()(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) const {
            if (a->priority != b->priority) {
                return a->priority < b->priority;
            }
            return a->scheduled_tick > b->scheduled_tick;
        }
    };
};

} // namespace Events






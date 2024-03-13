#pragma once
#include <string>
#include <unordered_set>
#include <vector>

struct graph_edge {
    int id;
    std::string company;
    std::string line;
    int departure_time;
    int arrival_time;
    std::string start_stop;
    std::string end_stop;
    int start_stop_id;
    int end_stop_id;
    float start_stop_lat;
    float start_stop_lon;
    float end_stop_lat;
    float end_stop_lon;
};

class astar {
public:
    struct result_stage {
        std::string line;
        std::string start_stop;
        int onboard_time;
        std::string end_stop;
        int offboard_time;
    };

    struct result {
        bool success;
        int total_vehicle_changes;
        int end_arrival_time;

        std::string start_stop;
        std::string end_stop;
        int start_stop_time;

        std::uint64_t total_cost;

        std::vector<result_stage> stages;
    };

    astar(const std::vector<graph_edge>& edges);

    void preprocess();
    void output_stop_names();
    std::unordered_set<std::string> get_lines_at_stop(int stop_id) const;
    result compute(int start_stop_id, int end_stop_id,
        bool optimize_time, int start_stop_time, std::string start_line = "");

private:
    using single_edge = std::tuple<int, int, std::string>;

    struct timetable_edge {
        int destination;
        std::vector<single_edge> times;
    };

    struct node_info {
        std::string stop_name;
        float lon, lat;

        int previous_node;
        single_edge* previous_edge;
        int veh_change_count;

        std::uint64_t total_cost;
        std::uint64_t current_cost;
        std::uint64_t estimated_cost;
        std::vector<timetable_edge> neighbors;
    };

    int get_max_node_id() const;
    float get_max_velocity() const;
    std::vector<node_info> construct_node_info(int node_max_id);
    std::uint64_t compute_heuristics(node_info& current, node_info& destination) const;
    std::tuple<std::uint64_t, single_edge*, bool> travel_cost(bool optimize_time,
        node_info& current, timetable_edge& next, const std::string& current_line) const;

    const std::vector<graph_edge>& _edges;
    float _max_velocity;
    std::vector<node_info> _nodes;
};

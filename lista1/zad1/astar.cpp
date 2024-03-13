#include "astar.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stack>
#include <queue>
#include <unordered_set>
#include <vector>


static const auto VEHICLE_CHANGE_COST = 1'000'000ULL;
static enum edge_tuple_idx {
    DEPARTURE,
    ARRIVAL,
    LINE_NAME
};


static inline float astar_get_distance(const graph_edge& edge) {
    return std::sqrt(
        (edge.end_stop_lon - edge.start_stop_lon) * (edge.end_stop_lon - edge.start_stop_lon)
        + (edge.end_stop_lat - edge.start_stop_lat) * (edge.end_stop_lat - edge.start_stop_lat)
    );
}

int astar::get_max_node_id() const
{
    int max_node_id = 0;

    for (const graph_edge& edge : _edges)
        max_node_id = std::max(max_node_id, std::max(edge.start_stop_id, edge.end_stop_id));

    return max_node_id;
}

float astar::get_max_velocity() const
{
    float velocity = 0;

    for (const graph_edge& edge : _edges) {
        float distance = astar_get_distance(edge);
        float this_velocity = distance / (edge.arrival_time - edge.departure_time);

        if (this_velocity > velocity && this_velocity != std::numeric_limits<float>::infinity()) {
            velocity = this_velocity;
        }
    }

    return velocity;
}

auto astar::construct_node_info(int node_max_id) -> std::vector<node_info>
{
    std::vector<node_info> nodes;
    nodes.resize(node_max_id + 1);

    for (const graph_edge& edge : _edges) {
        if (nodes[edge.start_stop_id].stop_name.empty()) {
            nodes[edge.start_stop_id].stop_name = edge.start_stop;
            nodes[edge.start_stop_id].lon = edge.start_stop_lon;
            nodes[edge.start_stop_id].lat = edge.start_stop_lat;
        }

        if (nodes[edge.end_stop_id].stop_name.empty()) {
            nodes[edge.end_stop_id].stop_name = edge.end_stop;
            nodes[edge.end_stop_id].lon = edge.end_stop_lon;
            nodes[edge.end_stop_id].lat = edge.end_stop_lat;
        }

        bool found_neighbor = false;
        for (auto& neighbor : nodes[edge.start_stop_id].neighbors) {
            if (neighbor.destination == edge.end_stop_id) {
                neighbor.times.push_back(std::make_tuple(
                    edge.departure_time, edge.arrival_time, edge.line));
                found_neighbor = true;
                break;
            }
        }

        if (!found_neighbor) {
            nodes[edge.start_stop_id].neighbors.push_back({
                .destination = edge.end_stop_id,
                .times = { std::make_tuple(
                    edge.departure_time, edge.arrival_time, edge.line) },
                });
        }
    }

    for (node_info& info : nodes) {
        info.total_cost = info.current_cost = info.estimated_cost = 0;
        info.previous_edge = nullptr;
        info.previous_node = 0;
        info.veh_change_count = 0;

        for (auto& neighbor_info : info.neighbors) {
            std::sort(neighbor_info.times.begin(), neighbor_info.times.end());
        }
    }

    return nodes;
}

std::uint64_t astar::compute_heuristics(node_info& current, node_info& destination) const
{
    float distance = std::sqrt(
        (destination.lon - current.lon) * (destination.lon - current.lon)
        + (destination.lat - current.lat) * (destination.lat - current.lat)
    );

    return static_cast<std::uint64_t>(distance / _max_velocity);
}

auto astar::travel_cost(bool optimize_time, node_info& current, timetable_edge& next,
    const std::string& current_line) const -> std::tuple<std::uint64_t, single_edge*, bool>
{
    int current_time = current.current_cost;
    if (current.previous_edge) {
        current_time = std::get<ARRIVAL>(*current.previous_edge);
    }

    int vehicle_change_cost = optimize_time ? 1 : VEHICLE_CHANGE_COST;

    // Perform bin-search to find nearest destination time
    int start = 0;
    int end = next.times.size();

    while (start < end) {
        int center = (start + end) / 2;
        int center_time = std::get<DEPARTURE>(next.times[center]);

        if (center_time < current_time) {
            start = center + 1;
        }
        else if (center_time >= current_time) {
            end = center;
        }
    }


    if (end < next.times.size()) {
        int departure_time = std::get<DEPARTURE>(next.times[end]);
        int arrival_time = std::get<ARRIVAL>(next.times[end]);
        int new_veh_change_count = current.veh_change_count;
        bool veh_changed = false;

        if (std::get<LINE_NAME>(next.times[end]) != current_line) {
            for (int i = end + 1; i < next.times.size(); ++i) {
                int this_departure_time = std::get<DEPARTURE>(next.times[i]);
                int this_arrival_time = std::get<ARRIVAL>(next.times[i]);

                if (optimize_time) {
                    // For optimize_time, we always want to have the best arrival time
                    if (this_departure_time != departure_time || this_arrival_time != arrival_time) {
                        break;
                    }
                }
                else {
                    // For disabled optimize_time, we may want to have a little
                    // worse arrival time, but still do not change lines
                    // (assuming that a single stop arrival_time == departure_time)

                    // Ignore this check for the very first node, to allow
                    // the chosen first line to "spread"
                    if (current.previous_edge != nullptr && this_departure_time != departure_time) {
                        break;
                    }
                }

                if (std::get<LINE_NAME>(next.times[i]) == current_line) {
                    return {
                        std::get<ARRIVAL>(next.times[i]) + new_veh_change_count * vehicle_change_cost,
                        &next.times[i], false
                    };
                }
            }

            ++new_veh_change_count;
            veh_changed = true;
        }

        return {
            std::get<ARRIVAL>(next.times[end]) + new_veh_change_count * vehicle_change_cost,
            &next.times[end], veh_changed
        };
    }
    else
        return { current_time + 24 * 60 * 60, nullptr, false };
}

astar::astar(const std::vector<graph_edge>& edges)
    : _edges(edges)
    , _max_velocity(0.f)
{
}

void astar::preprocess()
{
    std::cout << "Preprocesowanie danych grafu..." << std::endl;
    int node_max_id = get_max_node_id();
    float velocity = get_max_velocity();

    _nodes = construct_node_info(node_max_id);
}

void astar::output_stop_names()
{
    std::ofstream file("stops.txt");

    for (int i = 1; i < _nodes.size(); ++i) {
        file << i << '\t' << _nodes[i].stop_name << std::endl;
    }
}

std::unordered_set<std::string> astar::get_lines_at_stop(int stop_id) const
{
    std::unordered_set<std::string> out;
    if (stop_id <= 0 || stop_id > _nodes.size())
        return out;

    for (auto& neighbor : _nodes[stop_id].neighbors) {
        for (auto& time : neighbor.times) {
            out.insert(std::get<LINE_NAME>(time));
        }
    }
    return out;
}

auto astar::compute(int start_stop_id, int end_stop_id,
    bool optimize_time, int start_stop_time, std::string start_line) -> result
{
    result result;
    result.success = false;

    if (start_stop_id <= 0 || start_stop_id >= _nodes.size()
        || end_stop_id <= 0 || end_stop_id >= _nodes.size()
        || start_stop_id == end_stop_id) {

        return result;
    }

    std::cout << "Uruchamianie algorytmu A*..." << std::endl;
    _nodes[start_stop_id].current_cost = start_stop_time;
    _nodes[start_stop_id].estimated_cost = 0;
    _nodes[start_stop_id].total_cost
        = _nodes[start_stop_id].current_cost + _nodes[start_stop_id].estimated_cost;
    _nodes[start_stop_id].previous_edge = nullptr;
    _nodes[start_stop_id].previous_node = 0;
    _nodes[start_stop_id].veh_change_count = 0;

    std::priority_queue<std::pair<int, int>> open_nodes;
    std::unordered_set<int> open_nodes_set;
    std::unordered_set<int> closed_nodes;
    open_nodes.push(std::make_pair(-_nodes[start_stop_id].total_cost, start_stop_id));
    open_nodes_set.insert(start_stop_id);

    bool found_solution = false;
    while (!open_nodes.empty()) {
        auto [_, node_id] = open_nodes.top();
        if (node_id == end_stop_id) {
            std::cout << "Znaleziono rozwiazanie: "
                << time_to_str(std::get<ARRIVAL>(*_nodes[end_stop_id].previous_edge))
                << ", przesiadki: " << (_nodes[end_stop_id].veh_change_count - optimize_time)
                << std::endl;
            found_solution = true;
        }

        open_nodes.pop();
        open_nodes_set.erase(node_id);
        closed_nodes.insert(node_id);

        for (auto& neighbor : _nodes[node_id].neighbors) {
            int next_node_id = neighbor.destination;
            if (!open_nodes_set.contains(next_node_id) && !closed_nodes.contains(next_node_id)) {
                std::string current_line = start_line;
                if (_nodes[node_id].previous_edge) {
                    current_line = std::get<LINE_NAME>(*_nodes[node_id].previous_edge);
                }

                auto&& [travel_cost, edge_ptr, vehicle_change]
                    = this->travel_cost(optimize_time, _nodes[node_id], neighbor, current_line);

                if (edge_ptr) {
                    _nodes[next_node_id].estimated_cost
                        = compute_heuristics(_nodes[next_node_id], _nodes[end_stop_id]);
                    _nodes[next_node_id].current_cost = travel_cost;
                    _nodes[next_node_id].total_cost
                        = _nodes[next_node_id].estimated_cost + _nodes[next_node_id].current_cost;
                    _nodes[next_node_id].previous_node = node_id;
                    _nodes[next_node_id].previous_edge = edge_ptr;
                    _nodes[next_node_id].veh_change_count
                        = _nodes[node_id].veh_change_count + vehicle_change;

                    open_nodes_set.insert(next_node_id);
                    open_nodes.emplace(std::make_pair(-_nodes[next_node_id].total_cost, next_node_id));
                }
            }
            else {
                std::string current_line = start_line;
                if (_nodes[node_id].previous_edge) {
                    current_line = std::get<LINE_NAME>(*_nodes[node_id].previous_edge);
                }

                auto&& [travel_cost, edge_ptr, vehicle_change]
                    = this->travel_cost(optimize_time, _nodes[node_id], neighbor, current_line);
                bool better_route_found = _nodes[next_node_id].current_cost > travel_cost;

                // This is required to preserve consistence - to update cost,
                // there must be a route that contains smaller number of vehicle
                // changes. Otherwise, a line that arrives at this stop
                // a little earlier may be introduced, increasing a total number of
                // vehicle changes (A* has no clue about this behavior).
                if (!optimize_time) {
                    better_route_found = _nodes[next_node_id].current_cost > travel_cost + 86400;
                }

                if (edge_ptr && better_route_found) {
                    _nodes[next_node_id].current_cost = travel_cost;
                    _nodes[next_node_id].total_cost
                        = _nodes[next_node_id].estimated_cost + _nodes[next_node_id].current_cost;
                    _nodes[next_node_id].previous_node = node_id;
                    _nodes[next_node_id].previous_edge = edge_ptr;
                    _nodes[next_node_id].veh_change_count
                        = _nodes[node_id].veh_change_count + vehicle_change;

                    if (closed_nodes.contains(next_node_id)) {
                        open_nodes.emplace(
                            std::make_pair(-_nodes[next_node_id].total_cost, next_node_id));
                        open_nodes_set.insert(next_node_id);
                        closed_nodes.erase(next_node_id);
                    }
                }
            }
        }
    }

    if (!found_solution) {
        return result;
    }

    result.success = true;
    result.start_stop = _nodes[start_stop_id].stop_name;
    result.end_stop = _nodes[end_stop_id].stop_name;
    result.start_stop_time = start_stop_time;
    result.end_arrival_time = std::get<ARRIVAL>(*_nodes[end_stop_id].previous_edge);
    result.total_vehicle_changes = _nodes[end_stop_id].veh_change_count - optimize_time;
    result.total_cost = _nodes[end_stop_id].current_cost;

    std::stack<single_edge*> route;
    std::stack<int> route_nodes;
    result_stage current_stage;
    int current_node = end_stop_id;

    while (_nodes[current_node].previous_edge) {
        route.push(_nodes[current_node].previous_edge);

        current_node = _nodes[current_node].previous_node;
        route_nodes.push(current_node);
    }

    std::string current_line = "";
    int previous_arrival = 0;
    while (!route.empty() && !route_nodes.empty()) {
        auto&& [start_hr, end_hr, line] = *route.top();

        if (line != current_line) {
            if (!current_line.empty()) {
                current_stage.end_stop = _nodes[route_nodes.top()].stop_name;
                current_stage.offboard_time = previous_arrival;
                result.stages.push_back(current_stage);
            }

            current_stage.line = line;
            current_stage.start_stop = _nodes[route_nodes.top()].stop_name;
            current_stage.onboard_time = start_hr;

            current_line = line;
        }

        route.pop();
        route_nodes.pop();
        previous_arrival = end_hr;
    }

    current_stage.end_stop = _nodes[end_stop_id].stop_name;
    current_stage.offboard_time = previous_arrival;
    result.stages.push_back(current_stage);

    return result;
}

#include "astar.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stack>
#include <queue>
#include <unordered_set>
#include <vector>

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

    int total_cost;
    int current_cost;
    int estimated_cost;
    std::vector<timetable_edge> neighbors;
};

static int astar_get_max_node_id(const std::vector<graph_edge>& edges)
{
    int max_node_id = 0;

    for (const graph_edge& edge : edges)
        max_node_id = std::max(max_node_id, std::max(edge.start_stop_id, edge.end_stop_id));

    return max_node_id;
}

static inline float astar_get_distance(const graph_edge& edge) {
    return std::sqrt(
        (edge.end_stop_lon - edge.start_stop_lon) * (edge.end_stop_lon - edge.start_stop_lon)
        + (edge.end_stop_lat - edge.start_stop_lat) * (edge.end_stop_lat - edge.start_stop_lat)
    );
}

static float astar_get_max_velocity(const std::vector<graph_edge>& edges)
{
    float velocity = 0;

    for (const graph_edge& edge : edges) {
        float distance = astar_get_distance(edge);
        float this_velocity = distance / (edge.arrival_time - edge.departure_time);

        if (this_velocity > velocity && this_velocity != std::numeric_limits<float>::infinity()) {
            velocity = this_velocity;
        }
    }

    return velocity;
}

static std::vector<node_info> astar_construct_node_info(int node_max_id,
    const std::vector<graph_edge>& edges)
{
    std::vector<node_info> nodes;
    nodes.resize(node_max_id + 1);

    for (const graph_edge& edge : edges) {
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

static int astar_heuristics(float velocity,
    node_info& current, node_info& destination)
{
    float distance = std::sqrt(
        (destination.lon - current.lon) * (destination.lon - current.lon)
        + (destination.lat - current.lat) * (destination.lat - current.lat)
    );

    return static_cast<int>(distance / velocity);
}

static std::tuple<int, single_edge*, bool>
astar_travel_cost(node_info& current, timetable_edge& next, const std::string& current_line)
{
    //int current_time = current.current_cost;
    int current_time = current.current_cost;
    if (current.previous_edge) {
        current_time = std::get<1>(*current.previous_edge);
    }

    // Perform bin-search to find nearest destination time
    int start = 0;
    int end = next.times.size();

    while (start < end) {
        int center = (start + end) / 2;
        int center_time = std::get<0>(next.times[center]);

        if (center_time < current_time) {
            start = center + 1;
        }
        else if (center_time >= current_time) {
            end = center;
        }
    }


    if (end < next.times.size()) {
        int departure_time = std::get<0>(next.times[end]);
        int arrival_time = std::get<1>(next.times[end]);
        int new_veh_change_count = current.veh_change_count;
        bool veh_changed = false;

        if (std::get<2>(next.times[end]) != current_line) {
            for (int i = end + 1; i < next.times.size(); ++i) {
                int this_departure_time = std::get<0>(next.times[i]);
                int this_arrival_time = std::get<1>(next.times[i]);

                if (this_departure_time != departure_time || this_arrival_time != arrival_time) {
                    break;
                }

                if (std::get<2>(next.times[i]) == current_line) {
                    return { std::get<1>(next.times[i]) + new_veh_change_count, &next.times[i], false};
                }
            }

            ++new_veh_change_count;
            veh_changed = true;
        }

        return { std::get<1>(next.times[end]) + new_veh_change_count, &next.times[end], veh_changed};
    }
    else
        return { current_time + 24 * 60 * 60, nullptr, false };
}

static std::string astar_time_to_str(int time)
{
    std::string time_str = "00:00:00";
    int hours = time / 3600;
    int minutes = time / 60 - hours * 60;
    int seconds = time - minutes * 60 - hours * 3600;

    time_str[0] = (hours / 10) + '0';
    time_str[1] = (hours % 10) + '0';
    time_str[3] = (minutes / 10) + '0';
    time_str[4] = (minutes % 10) + '0';
    time_str[6] = (seconds / 10) + '0';
    time_str[7] = (seconds % 10) + '0';

    return time_str;
}

void astar_compute(const std::vector<graph_edge>& edges,
    int start_stop_id, int end_stop_id,
    bool optimize_time, int start_stop_time)
{
    std::cout << "Preprocesowanie danych grafu..." << std::endl;
    int node_max_id = astar_get_max_node_id(edges);
    float velocity = astar_get_max_velocity(edges);

    auto nodes = astar_construct_node_info(node_max_id, edges);
    nodes[start_stop_id].current_cost = start_stop_time;
    nodes[start_stop_id].estimated_cost = 0;
    nodes[start_stop_id].total_cost
        = nodes[start_stop_id].current_cost + nodes[start_stop_id].estimated_cost;

    std::priority_queue<std::pair<int, int>> open_nodes;
    std::unordered_set<int> open_nodes_set;
    std::unordered_set<int> closed_nodes;
    open_nodes.push(std::make_pair(-nodes[start_stop_id].total_cost, start_stop_id));
    open_nodes_set.insert(start_stop_id);

    std::cout << "Uruchamianie algorytmu A*..." << std::endl;
    while (!open_nodes.empty()) {
        auto [neg_cost, node_id] = open_nodes.top();
        if (node_id == end_stop_id) {
            std::cout << "Znaleziono rozwiazanie: "
                << astar_time_to_str(nodes[end_stop_id].current_cost)
                << std::endl;
        }

        open_nodes.pop();
        open_nodes_set.erase(node_id);
        closed_nodes.insert(node_id);

        for (auto& neighbor : nodes[node_id].neighbors) {
            int next_node_id = neighbor.destination;
            if (!open_nodes_set.contains(next_node_id) && !closed_nodes.contains(next_node_id)) {
                std::string current_line = "";
                if (nodes[node_id].previous_edge) {
                    current_line = std::get<2>(*nodes[node_id].previous_edge);
                }

                auto&& [travel_cost, edge_ptr, vehicle_change]
                    = astar_travel_cost(nodes[node_id], neighbor, current_line);

                nodes[next_node_id].estimated_cost
                    = astar_heuristics(velocity, nodes[next_node_id], nodes[end_stop_id]);
                nodes[next_node_id].current_cost = travel_cost;
                nodes[next_node_id].total_cost
                    = nodes[next_node_id].estimated_cost + nodes[next_node_id].current_cost;
                nodes[next_node_id].previous_node = node_id;
                nodes[next_node_id].previous_edge = edge_ptr;
                nodes[next_node_id].veh_change_count
                    = nodes[node_id].veh_change_count + vehicle_change;

                open_nodes_set.insert(next_node_id);
                open_nodes.emplace(std::make_pair(-nodes[next_node_id].total_cost, next_node_id));
            }
            else {
                std::string current_line = "";
                if (nodes[node_id].previous_edge) {
                    current_line = std::get<2>(*nodes[node_id].previous_edge);
                }

                auto&& [travel_cost, edge_ptr, vehicle_change]
                    = astar_travel_cost(nodes[node_id], neighbor, current_line);

                if (nodes[next_node_id].current_cost > travel_cost) {
                    nodes[next_node_id].current_cost = travel_cost;
                    nodes[next_node_id].total_cost
                        = nodes[next_node_id].estimated_cost + nodes[next_node_id].current_cost;
                    nodes[next_node_id].previous_node = node_id;
                    nodes[next_node_id].previous_edge = edge_ptr;
                    nodes[next_node_id].veh_change_count
                        = nodes[node_id].veh_change_count + vehicle_change;

                    if (closed_nodes.contains(next_node_id)) {
                        open_nodes.emplace(
                            std::make_pair(-nodes[next_node_id].total_cost, next_node_id));
                        open_nodes_set.insert(next_node_id);
                        closed_nodes.erase(next_node_id);
                    }
                }
            }
        }
    }

    std::stack<single_edge*> route;
    std::stack<int> route_nodes;
    int current_node = end_stop_id;

    while (nodes[current_node].previous_edge) {
        route.push(nodes[current_node].previous_edge);

        current_node = nodes[current_node].previous_node;
        route_nodes.push(current_node);
    }

    std::string current_line = "";
    int previous_arrival = 0;
    while (!route.empty() && !route_nodes.empty()) {
        auto&& [start_hr, end_hr, line] = *route.top();

        if (line != current_line) {
            if (!current_line.empty()) {
                std::cout << " - Wysiadz na przystanku " << nodes[route_nodes.top()].stop_name << std::endl;
                std::cout << " - Godzina: " << astar_time_to_str(previous_arrival) << std::endl;
            }

            std::cout << "Linia " << line << ": " << std::endl;
            std::cout << " - Wsiadz na przystanku " << nodes[route_nodes.top()].stop_name << std::endl;
            std::cout << " - Godzina: " << astar_time_to_str(start_hr) << std::endl;

            current_line = line;
        }

        route.pop();
        route_nodes.pop();
        previous_arrival = end_hr;
    }

    std::cout << " - Wysiadz na przystanku " << nodes[end_stop_id].stop_name << std::endl;
    std::cout << " - Godzina: " << astar_time_to_str(previous_arrival) << std::endl;
}

#pragma once
#include <string>
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

void astar_compute(const std::vector<graph_edge>& edges,
    int start_stop_id, int end_stop_id,
    bool optimize_time, int start_stop_time);

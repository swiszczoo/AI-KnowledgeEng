#include "astar.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static std::vector<graph_edge> edges;

static int time_to_int(char* buffer) {
    if (strlen(buffer) != 8) {
        return 0;
    }

    char mybuf[3] = { 0 };
    mybuf[0] = buffer[0];
    mybuf[1] = buffer[1];
    int hours = atoi(mybuf);
    mybuf[0] = buffer[3];
    mybuf[1] = buffer[4];
    int minutes = atoi(mybuf);
    mybuf[0] = buffer[6];
    mybuf[1] = buffer[7];
    int seconds = atoi(mybuf);

    return hours * 3600 + minutes * 60 + seconds;
}

bool read_data()
{
    std::ifstream file("connection_graph.csv");
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line);

    float fix_longitude = cos(51.08);

    char buffer[1024];
    while (!file.eof()) {
        std::getline(file, line);
        std::istringstream iss(line);
        graph_edge edge;

        iss.getline(buffer, 1024, ',');
        edge.id = atoi(buffer);
        iss.getline(buffer, 1024, ',');
        edge.company = buffer;
        iss.getline(buffer, 1024, ',');
        edge.line = buffer;
        iss.getline(buffer, 1024, ',');
        edge.departure_time = time_to_int(buffer);
        iss.getline(buffer, 1024, ',');
        edge.arrival_time = time_to_int(buffer);
        iss.getline(buffer, 1024, ',');
        edge.start_stop = buffer;
        iss.getline(buffer, 1024, ',');
        edge.end_stop = buffer;
        iss.getline(buffer, 1024, ',');
        edge.start_stop_lat = static_cast<float>(atof(buffer));
        iss.getline(buffer, 1024, ',');
        edge.start_stop_lon = static_cast<float>(atof(buffer));
        edge.start_stop_lon *= fix_longitude;

        iss.getline(buffer, 1024, ',');
        edge.end_stop_lat = static_cast<float>(atof(buffer));
        iss.getline(buffer, 1024, ',');
        edge.end_stop_lon = static_cast<float>(atof(buffer));
        edge.end_stop_lon *= fix_longitude;

        if (!file.eof()) {
            edges.emplace_back(std::move(edge));
        }
    }

    return true;
}

void hash_stop_ids()
{
    std::unordered_map<std::string, int> name_map;
    int index = 0;

    for (graph_edge& edge : edges) {
        auto start_it = name_map.find(edge.start_stop);
        if (start_it != name_map.end()) {
            edge.start_stop_id = start_it->second;
        }
        else {
            name_map[edge.start_stop] = edge.start_stop_id = ++index;
        }

        auto end_it = name_map.find(edge.end_stop);
        if (end_it != name_map.end()) {
            edge.end_stop_id = end_it->second;
        }
        else {
            name_map[edge.end_stop] = edge.end_stop_id = ++index;
        }
    }
}

int main()
{
#ifdef _WIN32
    system("chcp 65001 > NUL");
#endif
    edges.reserve(1'000'000);
    std::cout << "Wczytywanie pliku z danymi..." << std::endl;

    if (!read_data()) {
        std::cerr << "Wystapil blad!" << std::endl;
        return 1;
    }

    std::cout << "Hashowanie nazw przystankow..." << std::endl;
    hash_stop_ids();

    astar_compute(edges, 555, 641, true, 8 * 3600);
    return 0;
}

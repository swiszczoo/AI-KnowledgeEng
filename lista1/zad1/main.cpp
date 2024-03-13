#include "astar.h"
#include "utils.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static std::vector<graph_edge> edges;

static int time_to_int(const char* buffer) {
    if (strlen(buffer) < 8) {
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

    float fix_longitude = cos(51.08 / 180.0 * 3.141592653589);

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

    astar algorithm(edges);
    algorithm.preprocess();
    algorithm.output_stop_names();

    int start_stop_id, end_stop_id, start_stop_time;
    char temp;
    std::string temp_str;

    std::cout << "Podaj ID przystanku poczatkowego [plik stops.txt]: >";
    std::cin >> start_stop_id;
    std::cout << "Podaj ID przystanku koncowego [plik stops.txt]: >";
    std::cin >> end_stop_id;
    std::cout << "Optymalizacja czasu czy przesiadek [t/p]: >";
    std::cin >> temp;
    bool optimize_time = temp == 't';
    std::cout << "Czas pojawienia sie na przystanku poczatkowym [HH:MM]: >";
    std::cin >> temp_str;
    temp_str += ":00";
    start_stop_time = time_to_int(temp_str.c_str());

    auto time1 = std::chrono::steady_clock::now();
    astar::result result;
    result.total_cost = std::numeric_limits<decltype(result.total_cost)>().max();

    if (optimize_time) {
        result = algorithm.compute(
            start_stop_id, end_stop_id, optimize_time, start_stop_time);
    }
    else {
        for (auto& line : algorithm.get_lines_at_stop(start_stop_id)) {
            std::cout << "Uruchamianie dla linii " << line << "..." << std::endl;

            auto new_result = algorithm.compute(
                start_stop_id, end_stop_id, optimize_time, start_stop_time, line);

            if (new_result.total_cost < result.total_cost) {
                result = std::move(new_result);
            }
        }
    }
    auto time2 = std::chrono::steady_clock::now();

    std::cout << "Czas wykonywania algorytmu: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1)
        << std::endl;

    if (!result.success) {
        std::cout << "Nie znaleziono rozwiazania!" << std::endl;
        return 0;
    }

    std::cout << "Rozwiazanie:" << std::endl;
    std::cout << "Podroz z " << result.start_stop
        << " do " << result.end_stop
        << ", o godzinie " << time_to_str(result.start_stop_time)
        << std::endl;
    std::cout << "Czas dotarcia: " << time_to_str(result.end_arrival_time)
        << ", Przesiadki: " << result.total_vehicle_changes << std::endl;
    std::cout << "Funkcja kosztu: " << result.total_cost << std::endl;

    for (auto& stage : result.stages) {
        std::cout << "Linia " << stage.line << ": " << std::endl;
        std::cout << " - Wsiadz na przystanku " << stage.start_stop << std::endl;
        std::cout << " - Godzina: " << time_to_str(stage.onboard_time) << std::endl;
        std::cout << " - Wysiadz na przystanku " << stage.end_stop << std::endl;
        std::cout << " - Godzina: " << time_to_str(stage.offboard_time) << std::endl;
    }

    return 0;
}

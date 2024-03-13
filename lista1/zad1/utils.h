#pragma once
#include <string>

inline static std::string time_to_str(int time)
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

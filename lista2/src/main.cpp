#include <chrono>
#include <iostream>

#include "board.hpp"
#include "minimax.hpp"

static std::int64_t first_strategy(board::move m)
{
    return std::abs(m.x1 - m.x2) + std::abs(m.y1 - m.y2);
}

static std::int64_t second_strategy(board::move m)
{
    return static_cast<std::int64_t>(std::round(1000.0 * std::sqrt(
        (m.x1 - m.x2) * (m.x1 - m.x2) + (m.y1 - m.y2) * (m.y1 - m.y2))));
}

static std::int64_t third_strategy(board::move m)
{
    return std::max(std::abs(m.x1 - m.x2), std::abs(m.y1 - m.y2));
}

int main() {
    board brd;
    brd.set_metric_one(third_strategy);
    brd.set_metric_two(third_strategy);

    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            int tmp;
            std::cin >> tmp;
            brd.set_piece(x, y, tmp);
        }
    }

    auto moves = brd.get_legal_moves(1);

    minimax player1(1, 1);
    player1.set_depth(3);
    minimax player2(2, 2);
    player2.set_depth(3);

    auto start = std::chrono::steady_clock::now();

    int turn_counter = 0;
    std::uint64_t total_nodes = 0;
    while (!brd.get_winner()) {
        int nodes = 0;
        nodes += player1.make_next_move(brd);
        if (brd.get_winner()) continue;
        nodes += player2.make_next_move(brd);
        std::cerr << "Zakonczono ture " << (++turn_counter) << ", wezly: " << nodes << std::endl;
        total_nodes += nodes;
    }

    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            std::cout << brd.get_piece(x, y) << ' ';
        }
        std::cout << std::endl;
    }

    if (brd.get_winner() == 1) {
        std::cerr << "Wygral pierwszy gracz" << std::endl;
    }
    else {
        std::cerr << "Wygral drugi gracz" << std::endl;
    }

    auto duration = std::chrono::steady_clock::now() - start;
    std::cerr << "Czas: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        << std::endl;
    std::cerr << "Odwiedzone wezly: " << total_nodes << std::endl;
    return 0;
}

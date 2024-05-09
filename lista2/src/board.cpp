#include "board.hpp"

#include <array>
#include <bitset>
#include <cassert>

template <typename A, typename B>
inline std::pair<A, B> operator+(const std::pair<A, B>& first, const std::pair<A, B>& second)
{
    return { first.first + second.first, first.second + second.second };
}

template <typename A, typename B>
inline std::pair<A, B> operator*(const std::pair<A, B>& first, int scalar)
{
    return { scalar * first.first, scalar * first.second };
}

static const int camp_map[16][16] = {
    { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, },
};

board::board()
    : _metric_one([](move m) -> std::int64_t { return std::abs(m.x1 - m.x2) + std::abs(m.y1 - m.y2); })
    , _metric_two([](move m) -> std::int64_t { return std::abs(m.x1 - m.x2) + std::abs(m.y1 - m.y2); })
    , _player_one_ok_pieces(0)
    , _player_two_ok_pieces(0)
    , _heuristic_one(0)
    , _heuristic_two(0)
{
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            _pieces[x][y] = 0;
        }
    }
}

void board::set_metric_one(std::function<int64_t(move)> metric)
{
    _metric_one = metric;
}

void board::set_metric_two(std::function<int64_t(move)> metric)
{
    _metric_two = metric;
}

void board::set_piece(int x, int y, int player)
{
    assert(player >= 0 && player <= 2);
    assert(x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE);

    remove_piece(x, y);
    _pieces[x][y] = player;

    if (player == 1) {
        _player_one_pieces.push_back({ x, y });

        if (camp_map[x][y] == 1) {
            ++_player_one_ok_pieces;
        }

        // The farther I am, the worse my position is
        _heuristic_one -= _metric_one(move{ 0, 0, x, y });
        _heuristic_two -= _metric_two(move{ 0, 0, x, y });
    }
    else if (player == 2) {
        _player_two_pieces.push_back({ x, y });

        if (camp_map[x][y] == 2) {
            ++_player_one_ok_pieces;
        }

        // The farther my opponent is, the better my position is
        _heuristic_one += _metric_one(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x, y });
        _heuristic_two += _metric_two(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x, y });
    }
}

int board::get_piece(int x, int y) const
{
    assert(x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE);
    return _pieces[x][y];
}

void board::remove_piece(int x, int y)
{
    if (_pieces[x][y] == 1) {
        for (auto it = _player_one_pieces.begin(); it != _player_one_pieces.end(); ++it) {
            if (it->first == x && it->second == y) {
                _player_one_pieces.erase(it);
                break;
            }
        }

        if (camp_map[x][y] == 1) {
            --_player_one_ok_pieces;
        }

        _heuristic_one += _metric_one(move{ 0, 0, x, y });
        _heuristic_two += _metric_two(move{ 0, 0, x, y });
    }
    else if (_pieces[x][y] == 2) {
        for (auto it = _player_two_pieces.begin(); it != _player_two_pieces.end(); ++it) {
            if (it->first == x && it->second == y) {
                _player_two_pieces.erase(it);
                break;
            }
        }

        if (camp_map[x][y] == 2) {
            --_player_two_ok_pieces;
        }

        _heuristic_one -= _metric_one(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x, y });
        _heuristic_two -= _metric_two(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x, y });
    }

    _pieces[x][y] = 0;
}

void board::move_piece(int x1, int y1, int x2, int y2)
{
    assert(_pieces[x1][y1]);
    assert(!_pieces[x2][y2]);

    decltype(_player_one_pieces)::iterator it, end_it;
    if (_pieces[x1][y1] == 1) {
        it = _player_one_pieces.begin();
        end_it = _player_one_pieces.end();

        if (camp_map[x1][y1] == 1) {
            --_player_one_ok_pieces;
        }

        _heuristic_one += _metric_one(move{ 0, 0, x1, y1 });
        _heuristic_two += _metric_two(move{ 0, 0, x1, y1 });
        _heuristic_one -= _metric_one(move{ 0, 0, x2, y2 });
        _heuristic_two -= _metric_two(move{ 0, 0, x2, y2 });
    }
    else if (_pieces[x1][y1] == 2) {
        it = _player_two_pieces.begin();
        end_it = _player_two_pieces.end();

        if (camp_map[x1][y1] == 2) {
            --_player_two_ok_pieces;
        }

        _heuristic_one -= _metric_one(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x1, y1 });
        _heuristic_two -= _metric_two(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x1, y1 });
        _heuristic_one += _metric_one(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x2, y2 });
        _heuristic_two += _metric_two(move{ BOARD_SIZE - 1, BOARD_SIZE - 1, x2, y2 });
    }

    for (; it != end_it; ++it) {
        if (it->first == x1 && it->second == y1) {
            it->first = x2;
            it->second = y2;
        }
    }

    _pieces[x2][y2] = _pieces[x1][y1];
    _pieces[x1][y1] = 0;

    if (camp_map[x2][y2] == 1 && _pieces[x2][y2] == 1) {
        ++_player_one_ok_pieces;
    }

    if (camp_map[x2][y2] == 2 && _pieces[x2][y2] == 2) {
        ++_player_two_ok_pieces;
    }
}

void board::undo_move(int x1, int y1, int x2, int y2)
{
    move_piece(x2, y2, x1, y1);
}

static const auto single_moves = std::array{
    std::pair { -1, -1 },
    std::pair { -1, 0 },
    std::pair { -1, 1 },
    std::pair { 0, -1 },
    std::pair { 0, 1 },
    std::pair { 1, -1 },
    std::pair { 1, 0 },
    std::pair { 1,1 },
};

inline int to_pos(std::pair<int, int> par) {
    return par.first * BOARD_SIZE + par.second;
}

inline bool in_board(std::pair<int, int> par) {
    return par.first >= 0 && par.first < BOARD_SIZE
        && par.second >= 0 && par.second < BOARD_SIZE;
}

auto board::get_legal_moves(int player) const -> std::vector<move>
{
    assert(player == 1 || player == 2);

    std::vector<move> moves;
    auto& pieces = player == 1 ? _player_one_pieces : _player_two_pieces;

    for (auto& piece : pieces) {
        std::bitset<BOARD_SIZE* BOARD_SIZE> visited;
        visited.set(to_pos(piece));

        for (auto& move : single_moves) {
            auto dest = piece + move;
            auto jump_dest = dest + move;

            if (!in_board(dest)) {
                continue;
            }

            if (_pieces[dest.first][dest.second] == 0) {
                moves.push_back(board::move{
                    .x1 = piece.first, .y1 = piece.second,
                    .x2 = dest.first, .y2 = dest.second,
                    });
            }
            else if (in_board(jump_dest) && _pieces[jump_dest.first][jump_dest.second] == 0) {
                rec_add_jump_moves(moves, visited, piece, jump_dest);
            }
        }
    }

    return moves;
}

int board::get_winner() const
{
    if (_player_one_ok_pieces == _player_one_pieces.size()) return 1;
    if (_player_two_ok_pieces == _player_two_pieces.size()) return 2;

    return 0;
}

std::int64_t board::get_heuristic_one() const
{
    int winner = get_winner();
    if (winner == 1) return LLONG_MAX;
    else if (winner == 2) return LLONG_MIN;

    return _heuristic_one;
}

std::int64_t board::get_heuristic_two() const
{
    int winner = get_winner();
    if (winner == 1) return LLONG_MAX;
    else if (winner == 2) return LLONG_MIN;

    return _heuristic_two;
}

std::uint64_t board::hash_position() const
{
    std::uint64_t result = 0;
    std::uint64_t multiplicand = 1;

    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            result += multiplicand * static_cast<std::uint64_t>(_pieces[x][y]);
            multiplicand *= 3;
        }
    }

    return result;
}

void board::copy_position(std::uint8_t out[BOARD_SIZE][BOARD_SIZE])
{
    std::memcpy(out, _pieces, sizeof(_pieces));
}

bool board::compare_position(std::uint8_t in[BOARD_SIZE][BOARD_SIZE])
{
    return std::memcmp(in, _pieces, sizeof(_pieces)) == 0;
}

void board::rec_add_jump_moves(std::vector<move>& out,
    std::bitset<BOARD_SIZE* BOARD_SIZE>& visited,
    std::pair<int, int> from, std::pair<int, int> pos) const
{
    visited.set(to_pos(pos));
    out.push_back(board::move{
        .x1 = from.first, .y1 = from.second,
        .x2 = pos.first, .y2 = pos.second,
        });

    for (auto& move : single_moves) {
        auto dest = pos + move;
        auto jump_dest = dest + move;

        // We don't have to check if dest is in range
        if (!in_board(jump_dest)) {
            continue;
        }

        if (visited.test(to_pos(jump_dest))) {
            continue;
        }

        if (_pieces[dest.first][dest.second] && !_pieces[jump_dest.first][jump_dest.second]) {
            rec_add_jump_moves(out, visited, from, jump_dest);
        }
    }
}


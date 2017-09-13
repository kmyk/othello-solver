#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <tuple>
#define repeat(i, n) for (int i = 0; (i) < int(n); ++(i))
#define whole(x) begin(x), end(x)
using namespace std;
template <class T> inline void setmax(T & a, T const & b) { a = max(a, b); }

typedef uint64_t bitboard_t;

bitboard_t to_bitboard(int y, int x) {
    assert(0 <= y && y < 8);
    assert(0 <= x && x < 8);
    return 1ull << (y * 8 + x);
}
pair<int, int> from_bitboard(bitboard_t t) {
    repeat (y, 8) {
        repeat (x, 8) {
            if (to_bitboard(y, x) & t) {
                return { y, x };
            }
        }
    }
    return { -1, - 1 };
}

template <typename Shift>
bitboard_t get_mobility_one_direction(bitboard_t black, bitboard_t white, bitboard_t mask, Shift shift) {
    bitboard_t w = white & mask;
    bitboard_t t = w & shift(black);
    t |= w & shift(t);
    t |= w & shift(t);
    t |= w & shift(t);
    t |= w & shift(t);
    t |= w & shift(t);
    bitboard_t blank = ~ (black | white);
    return blank & shift(t);
}
bitboard_t get_mobility(bitboard_t black, bitboard_t white) {
    bitboard_t mobility = 0;
    mobility |= get_mobility_one_direction(black, white, 0x7e7e7e7e7e7e7e7e, [](bitboard_t t) { return t >> 1; }); // right
    mobility |= get_mobility_one_direction(black, white, 0x007e7e7e7e7e7e00, [](bitboard_t t) { return t << 7; }); // up right
    mobility |= get_mobility_one_direction(black, white, 0x00ffffffffffff00, [](bitboard_t t) { return t << 8; }); // up
    mobility |= get_mobility_one_direction(black, white, 0x007e7e7e7e7e7e00, [](bitboard_t t) { return t << 9; }); // up left
    mobility |= get_mobility_one_direction(black, white, 0x7e7e7e7e7e7e7e7e, [](bitboard_t t) { return t << 1; }); // left
    mobility |= get_mobility_one_direction(black, white, 0x007e7e7e7e7e7e00, [](bitboard_t t) { return t >> 7; }); // down left
    mobility |= get_mobility_one_direction(black, white, 0x00ffffffffffff00, [](bitboard_t t) { return t >> 8; }); // down
    mobility |= get_mobility_one_direction(black, white, 0x007e7e7e7e7e7e00, [](bitboard_t t) { return t >> 9; }); // down right
    return mobility;
}

template <typename Shift>
inline bitboard_t get_reversed_one_direction(bitboard_t black, bitboard_t white, bitboard_t black_move, bitboard_t mask, Shift shift) {
    bitboard_t wh = white & mask;
    bitboard_t m1 = shift(black_move);
    bitboard_t m2, m3, m4, m5, m6;
    bitboard_t rev = 0;
    if ( (m1 & wh) != 0 ) {
        if ( ((m2 = shift(m1)) & wh) == 0 ) {
            if ( (m2 & black) != 0 ) rev |= m1;
        } else if ( ((m3 = shift(m2)) & wh) == 0 ) {
            if ( (m3 & black) != 0 ) rev |= m1 | m2;
        } else if ( ((m4 = shift(m3)) & wh) == 0 ) {
            if ( (m4 & black) != 0 ) rev |= m1 | m2 | m3;
        } else if ( ((m5 = shift(m4)) & wh) == 0 ) {
            if ( (m5 & black) != 0 ) rev |= m1 | m2 | m3 | m4;
        } else if ( ((m6 = shift(m5)) & wh) == 0 ) {
            if ( (m6 & black) != 0 ) rev |= m1 | m2 | m3 | m4 | m5;
        } else {
            if ( (shift(m6) & black) != 0 ) rev |= m1 | m2 | m3 | m4 | m5 | m6;
        }
    }
    return rev;
}
bitboard_t get_reversed(bitboard_t black, bitboard_t white, bitboard_t black_move) {
    bitboard_t reversed = 0;
    reversed |= get_reversed_one_direction(black, white, black_move, 0x7f7f7f7f7f7f7f7f, [](bitboard_t t) { return t >> 1; }); // right
    reversed |= get_reversed_one_direction(black, white, black_move, 0x7f7f7f7f7f7f7f7f, [](bitboard_t t) { return t << 7; }); // up right
    reversed |= get_reversed_one_direction(black, white, black_move, 0xffffffffffffffff, [](bitboard_t t) { return t << 8; }); // up
    reversed |= get_reversed_one_direction(black, white, black_move, 0xfefefefefefefefe, [](bitboard_t t) { return t << 9; }); // up left
    reversed |= get_reversed_one_direction(black, white, black_move, 0xfefefefefefefefe, [](bitboard_t t) { return t << 1; }); // left
    reversed |= get_reversed_one_direction(black, white, black_move, 0xfefefefefefefefe, [](bitboard_t t) { return t >> 7; }); // down left
    reversed |= get_reversed_one_direction(black, white, black_move, 0xffffffffffffffff, [](bitboard_t t) { return t >> 8; }); // down
    reversed |= get_reversed_one_direction(black, white, black_move, 0x7f7f7f7f7f7f7f7f, [](bitboard_t t) { return t >> 9; }); // down right
    return reversed;
}

int bitboard_popcount(bitboard_t t) {
    return __builtin_popcountll(t);
}

template <typename F>
int negaalpha_search(bitboard_t black, bitboard_t white, int alpha, int beta, int depth, F evaluate) {
    if (depth == 0) {
        return evaluate(black, white);
    } else {
        bitboard_t y = get_mobility(black, white);
        for (bitboard_t x = y & - y; x; x = y & (~ y + (x << 1))) { // x is a singleton and a subset of y
            bitboard_t reversed = get_reversed(black, white, x);
            setmax(alpha, - negaalpha_search(white | reversed, black | reversed | x, - beta, - alpha, depth - 1, evaluate));
            if (alpha >= beta) break;
        }
        return alpha;
    }
}
template <typename F>
bitboard_t negaalpha_move(bitboard_t black, bitboard_t white, int alpha, int beta, int depth, F evaluate) {
    assert (depth != 0);
    bitboard_t m = 0;
    bitboard_t y = get_mobility(black, white);
    for (bitboard_t x = y & - y; x; x = y & (~ y + (x << 1))) { // x is a singleton and a subset of y
        bitboard_t reversed = get_reversed(black, white, x);
        int next_alpha = - negaalpha_search(white | reversed, black | reversed | x, - beta, - alpha, depth - 1, evaluate);
        if (alpha < next_alpha) {
            alpha = next_alpha;
            m = x;
        }
        if (alpha >= beta) break;
    }
    assert (m);
    return m;
}

string show_bitboard(bitboard_t black, bitboard_t white, bitboard_t dot = 0) {
    ostringstream oss;
    oss << "  A B C D E F G H" << endl;
    repeat (y, 8) {
        oss << (y + 1) << ' ';
        repeat (x, 8) {
            char c = '-';
            if (black & to_bitboard(y, x)) {
                c = '*';
            } else if (white & to_bitboard(y, x)) {
                c = 'O';
            } else if (dot & to_bitboard(y, x)) {
                c = '.';
            }
            oss << c << ' ';
        }
        oss << (y + 1) << endl;
    }
    oss << "  A B C D E F G H" << endl;
    return oss.str();
}
const bitboard_t initial_black = to_bitboard(3, 4) | to_bitboard(4, 3);
const bitboard_t initial_white = to_bitboard(3, 3) | to_bitboard(4, 4);

pair<int, int> read_vertex(string s) {
    if (not (s.length() == 2)) return { -1, -1 };
    s[0] = tolower(s[0]);
    if (not ('a' <= s[0] and s[0] <= 'h')) return { -1, -1 };
    if (not ('1' <= s[1] and s[1] <= '8')) return { -1, -1 };
    return { s[1] - '1', s[0] - 'a' };
}
string show_vertex(int y, int x) {
    return string() + "abcdefgh"[x] + "12345678"[y];
}

const bitboard_t bitboard_corner
    = to_bitboard(0, 0)
    | to_bitboard(0, 7)
    | to_bitboard(7, 0)
    | to_bitboard(7, 7);
const bitboard_t bitboard_around_corner
    = to_bitboard(0, 1)
    | to_bitboard(1, 0)
    | to_bitboard(1, 1)
    | to_bitboard(0, 6)
    | to_bitboard(1, 7)
    | to_bitboard(1, 6)
    | to_bitboard(7, 1)
    | to_bitboard(6, 0)
    | to_bitboard(6, 1)
    | to_bitboard(7, 6)
    | to_bitboard(6, 7)
    | to_bitboard(6, 6);
bitboard_t genmove(bitboard_t black, bitboard_t white) {
    constexpr int inf = 1e9+7;
    return negaalpha_move(black, white, - inf, inf, 8, [](bitboard_t black, bitboard_t white) {
        int score = 0;
        score += bitboard_popcount(black) * 1000;
        score -= bitboard_popcount(white) * 1000;
        score += bitboard_popcount(black & bitboard_corner) * 50000;
        score -= bitboard_popcount(white & bitboard_corner) * 50000;
        score -= bitboard_popcount(black & bitboard_around_corner) * 10000;
        score += bitboard_popcount(white & bitboard_around_corner) * 10000;
        score += bitboard_popcount(get_mobility(black, white)) * 1000;
        score -= bitboard_popcount(get_mobility(white, black)) * 1000;
        return score;
    });
}

bool swap_by_color_string(bitboard_t & black, bitboard_t & white, string color) {
    if (color == "b" or color == "black") {
        // pass
        return true;
    } else if (color == "w" or color == "white") {
        swap(black, white);
        return true;
    } else {
        return false;
    }
}

int main() {
    bitboard_t black = initial_black;
    bitboard_t white = initial_white;
    while (true) {
        cout.flush();
        istringstream iss; {
            string line; getline(cin, line);
            iss = istringstream(line);
        }
        string command; iss >> command;

        // play color vertex
        if (command == "play") {
            string color; iss >> color;
            if (not swap_by_color_string(black, white, color)) {
                cout << "? syntax error (wrong color)" << endl;
                continue;
            }
            string vertex; iss >> vertex;
            int y, x; tie(y, x) = read_vertex(vertex);
            if (y == -1 or x == -1) {
                cout << "? syntax error (wrong vertex)" << endl;
                continue;
            }
            bitboard_t m = to_bitboard(y, x);
            if (not (m & get_mobility(black, white))) {
                cout << "? illegal move" << endl;
                continue;
            }
            bitboard_t reversed = get_reversed(black, white, m);
            black ^= reversed | m;
            white ^= reversed;
            swap_by_color_string(black, white, color);
            cout << "= " << endl;

        // genmove color
        } else if (command == "genmove") {
            string color; iss >> color;
            if (not swap_by_color_string(black, white, color)) {
                cout << "? syntax error (wrong color)" << endl;
                continue;
            }
            bitboard_t m = genmove(black, white);
            assert (m & get_mobility(black, white));
            bitboard_t reversed = get_reversed(black, white, m);
            black ^= reversed | m;
            white ^= reversed;
            swap_by_color_string(black, white, color);
            int y, x; tie(y, x) = from_bitboard(m);
            cout << "= " << show_vertex(y, x) << endl;

        // showboard
        } else if (command == "showboard") {
            bitboard_t mobility = get_mobility(black, white);
            cout << "= " << show_bitboard(black, white, mobility) << endl;

        // setboard board
        } else if (command == "setboard") {
            string board; iss >> board;
            if (board.size() != 64
                    or count_if(whole(board), [](char c) { return c == '*' or c == 'O' or c == '-'; }) != 64) {
                cout << "? syntax error" << endl;
                continue;
            }
            black = white = 0;
            repeat (y, 8) {
                repeat (x, 8) {
                    char c = board[y * 8 + x];
                    if (c == '*') {
                        black |= to_bitboard(y, x);
                    } else if (c == 'O') {
                        white |= to_bitboard(y, x);
                    } else {
                        assert (c == '-');
                    }
                }
            }
            cout << "= " << endl;

        } else if (command.empty()) {
            // nop
        } else {
            cout << "? unknown command" << endl;
        }
    }
    return 0;
}

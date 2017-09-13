#!/usr/bin/env python3
import sys
import time
import subprocess
import curses

class GTPException(Exception):
    pass
class GTPAdapter(object):
    def __init__(self, path):
        self.proc = subprocess.Popen(
                [ path ],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=sys.stderr,
                )

    def __enter__(self):
        # self.proc.__enter__()
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        # self.proc.__exit__(exc_type, exc_value, traceback)
        self.proc.kill()

    def _issue_command(self, *command):
        self.proc.stdin.write((' '.join(command) + '\n').encode())
        self.proc.stdin.flush()
        s = self.proc.stdout.readline().decode()
        if s.startswith('= '):
            if s[2 :].rstrip():
                return s[2 :].rstrip()
        elif s.startswith('? '):
            raise GTPException(s[2 :].rstrip())
        else:
            assert False

    def protocol_version(self):
        return self._issue_command('protocol_version')
    def name(self):
        return self._issue_command('name')
    def version(self):
        return self._issue_command('version')
    def known_command(self, command_name):
        to_bool = { 'True': True, 'False': False }
        return to_bool[self._issue_command('known_command', command_name).capitalize()]
    def list_commands(self):
        return self._issue_command('list_commands').split()
    def quit(self):
        self._issue_command('quit')
    def boardsize(self, size):
        raise NotImplementedError
    def clear_board(self):
        self._issue_command('clear_board')
    def komi(self):
        raise NotImplementedError
    def play(self, color, vertex):
        self._issue_command('play', color, vertex)
    def genmove(self, color):
        return self._issue_command('genmove', color)

    def ext_showboard(self):
        return self._issue_command('ext/showboard')
    def ext_ispass(self, color):
        to_bool = { 'True': True, 'False': False }
        return to_bool[self._issue_command('ext/ispass', color).capitalize()]
    def ext_setboard(self, board):
        raise NotImplementedError

def flip_color(color):
    colors = [ 'black', 'white' ]
    return colors[colors.index(color) ^ 1]

def vertex_to_indices(vertex):
    x = 'abcdefgh'.index(vertex[0].lower())
    y = '12345678'.index(vertex[1])
    return y, x
def indices_to_vertex(y, x):
    return 'abcdefgh'[x] + '12345678'[y]

COLOR_PAIR_WHITE_ON_BLACK = 1
COLOR_PAIR_BLACK_ON_WHITE = 2
COLOR_PAIR_BLACK_ON_GREEN = 3
COLOR_PAIR_WHITE_ON_GREEN = 4

def blit_board(**kwargs):
    board = kwargs['board']
    player = kwargs['player']
    cursor_y = kwargs['cursor_y']
    cursor_x = kwargs['cursor_x']
    stdscr = kwargs['stdscr']
    for y in range(8):
        for x in range(8):
            for dy in [ -1, 0, 1 ]:
                for dx in [ -1, 0, 1 ]:
                    if dy and not dx:
                        c = '-'
                    elif not dy and dx:
                        c = '|'
                    else:
                        c = '+'
                    stdscr.addch(1 + 2 * y + dy, 1 + 2 * x + dx, c, curses.color_pair(COLOR_PAIR_BLACK_ON_GREEN))
    for y in range(8):
        for x in range(8):
            b = board[y * 8 + x]
            if b == '*':
                color = COLOR_PAIR_WHITE_ON_BLACK
            elif b == 'O':
                color = COLOR_PAIR_BLACK_ON_WHITE
            else:
                assert b == '-'
                if player == 'black':
                    color = COLOR_PAIR_BLACK_ON_GREEN
                else:
                    color = COLOR_PAIR_WHITE_ON_GREEN
            c = ' '
            if ( y, x ) == ( cursor_y, cursor_x ):
                c = '*'
            stdscr.addch(1 + 2 * y, 1 + 2 * x, c, curses.color_pair(color))

def main(args):
    stdscr = curses.initscr()
    try:
        # init
        curses.start_color()
        curses.init_pair(COLOR_PAIR_WHITE_ON_BLACK, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(COLOR_PAIR_BLACK_ON_WHITE, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(COLOR_PAIR_BLACK_ON_GREEN, curses.COLOR_BLACK, curses.COLOR_GREEN)
        curses.init_pair(COLOR_PAIR_WHITE_ON_GREEN, curses.COLOR_WHITE, curses.COLOR_GREEN)
        curses.curs_set(0)  # cursor is invisible
        curses.cbreak()  # no buffering
        curses.noecho()
        stdscr.keypad(True)  # make getch() to return symbols like KEY_LEFT
        cursor_y = 0
        cursor_x = 0

        # main loop
        with GTPAdapter(args.solver) as solver:
            turn = 'black'
            while True:

                # update
                board = solver.ext_showboard()
                if solver.ext_ispass(turn):
                    turn = flip_color(turn)
                elif turn != args.player:
                    stdscr.clear()
                    blit_board(board=board, player=args.player, cursor_y=cursor_y, cursor_x=cursor_x, stdscr=stdscr)
                    stdscr.refresh()
                    time.sleep(1)
                    solver.genmove(turn)
                    turn = flip_color(turn)
                    board = solver.ext_showboard()

                # render
                stdscr.clear()
                blit_board(board=board, player=args.player, cursor_y=cursor_y, cursor_x=cursor_x, stdscr=stdscr)
                stdscr.refresh()

                # input
                key = stdscr.getch()
                print(repr(key), file=sys.stderr)
                if key in ( curses.KEY_ENTER, ord(' '), ord('\n'), ord('\r') ):
                    if turn == args.player:
                        try:
                            solver.play(args.player, indices_to_vertex(cursor_y, cursor_x))
                            turn = flip_color(turn)
                        except GTPException as e:
                            print(e, file=sys.stderr)
                elif key == curses.KEY_UP:
                    cursor_y -= 1
                elif key == curses.KEY_DOWN:
                    cursor_y += 1
                elif key == curses.KEY_RIGHT:
                    cursor_x += 1
                elif key == curses.KEY_LEFT:
                    cursor_x -= 1
                cursor_y %= 8
                cursor_x %= 8

    finally:
        curses.endwin()

if __name__ == '__main__':
    import argparse
    import random
    parser = argparse.ArgumentParser()
    parser.add_argument('--solver',
            metavar='path',
            default='./a.out',
            )
    parser.add_argument('--player',
            choices=[ 'black', 'white' ],
            default=random.choice([ 'black', 'white' ]),
            )
    args = parser.parse_args()
    main(args)

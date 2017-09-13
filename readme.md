# Othello Solver

A very simple AI for othello.

It is originally made for a CTF challenge: [ASIS CTF Finals 2017: Chaoyang District](https://ctftime.org/task/4585).

## How to use

Compile with:

```
$ g++ -std=c++11 ai.cpp
```

Then, run a UI with curses:

```
$ python3 ui.py
```

## Note

It uses:

-   bitboard
-   nega-alpha pruning
-   weights of cells for evaluation function
-   GTP (Go Text Protocol)

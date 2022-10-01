#include <Arduino.h>
#include <SPI.h>
#include <bitset>
#include <map>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>

#define ROW 11
#define COL 17

class A_star{
    public:
        A_star();
        void start();
        typedef std::pair<int, int> Pair;
        typedef std::pair<double, std::pair<int, int> > pPair;
        struct cell {
            int parent_i, parent_j;
            // f = g + h
            double f, g, h;
        };
        std::stack<Pair> tracePath(cell cellDetails[][COL], Pair dest);
        std::stack<Pair> aStarSearch(int grid[][COL], Pair src, Pair dest);
    private:
        bool isValid(int row, int col);
        bool isUnBlocked(int grid[][COL], int row, int col);
        bool isDestination(int row, int col, Pair dest);
        double calculateHValue(int row, int col, Pair dest);
};





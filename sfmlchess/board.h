#pragma once
#include <iostream>
#include <list>
#include <map>
#include<vector>

#include "piece.h"

using namespace std;


class board {
public:

	board();
	board(vector<piece> boardvar);

	piece getPiece(int const& i);

private:

	vector<piece> layout;

};
#pragma once
#include <iostream>
#include <list>
#include <map>

#include "piece.h"

using namespace std;


class board {
public:

	

	board();
	board(int const& boardvar);

	piece getPiece(int const& i);

private:

	piece layout;

};
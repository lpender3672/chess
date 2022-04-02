#include <iostream>
#include <list>
#include <map>

#include "board.h"

board::board() {
	int boardvar[64] = { 1, 2, 3, 5, 4, 3, 2, 1,
						 6, 6, 6, 6, 6, 6, 6, 6,
						 0, 0, 0, 0, 0, 0, 0, 0,
						 0, 0, 0, 0, 0, 0, 0, 0,
						 0, 0, 0, 0, 0, 0, 0, 0,
						 0, 0, 0, 0, 0, 0, 0, 0,
						12,12,12,12,12,12,12,12,
						 7, 8, 9,10,11, 9, 8, 7 };
}

board::board(int const& _boardvar) {

	boardvar = _boardvar;
}

std::list<std::list<std::list<int>>> vectorflipper(int mov_v2[2], int pos_v2[2], bool infinite) {
	int unit_identity_list[4][2] = { {1,1}, {-1,1}, {1,-1}, {-1, -1} };
	int repeat = 1;
	if (infinite) {
		repeat = 8;
	}

	std::list<std::list<std::list<int>>>  moves;

	for (int v2 = 0; v2 < 4; v2++)
	{

		unit_identity_list[v2];
		std::list<std::list<int>> move_plane;

		for (int i = 0; i < repeat; i++)
		{
			std::list<int> move_v2;
			int x = (i + mov_v2[0]) * unit_identity_list[v2][0] + pos_v2[0];
			move_v2.push_back(x);
			int y = (i + mov_v2[1]) * unit_identity_list[v2][1] + pos_v2[1];
			move_v2.push_back(y);
			move_plane.push_back(move_v2);
		}
		moves.push_back(move_plane);

	}
	return moves;
}


std::map<int, std::map<std::string, std::string>> getmap() {

	std::map<int, std::map<std::string, std::string>> identity;

	std::string piecenames[6] = { "rook", "knight", "bishop", "king", "queen", "pawn" };

	for (int i = 0; i < 12; i++)
	{
		std::string colour;
		std::map<std::string, std::string> piece;
		if (i < 6) {
			colour = "white";
		}
		else {
			colour = "black";
		}

		piece["colour"] = colour;
		piece["name"] = piecenames[i % 6];

		identity[i] = piece;
	}
	return identity;
}
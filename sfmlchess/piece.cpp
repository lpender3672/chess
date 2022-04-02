

#include <iostream>
#include <list>
#include <map>

#include "piece.h"

piece::piece() {
	int id;
}

piece::piece(int const& _id) {
	std::string piecenames[6] = { "rook", "knight", "bishop", "king", "queen", "pawn" };
	id = _id;
	name = piecenames[_id];
}

int piece::getID() {
	return id;
}

std::string piece::getName()  {
	return name;
}
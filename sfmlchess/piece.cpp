

#include <iostream>
#include <list>
#include <map>

#include "piece.h"

piece::piece() {
	id = 0;
}

piece::piece(int const& _id) {
	std::string piecenames[6] = { "rook", "knight", "bishop", "king", "queen", "pawn" };
	id = _id;
	name = piecenames[_id];

	if (_id == 0) {
		name = "none";
	}
	else if (_id <= 6) {
		colour = "white";
	}
	else {
		colour = "black";
	}
}

int piece::getID() {
	return id;
}

std::string piece::getName()  {
	return name;
}
#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "board.h"

board::board() {

	layout = vector<piece>{ piece(3), piece(4), piece(5), piece(2), piece(1), piece(5), piece(4), piece(3),
							piece(6), piece(6), piece(6), piece(6), piece(6), piece(6), piece(6), piece(6),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12),
							piece(9), piece(11), piece(10), piece(7), piece(8), piece(10), piece(11), piece(9) };
}

board::board(vector<piece> _layout, sf::Sprite _boardSprite, sf::Sprite _pieceSprites) {

	layout = _layout;
	boardSprite = _boardSprite;
	pieceSprites = _pieceSprites;

	unitWidth = boardSprite.getGlobalBounds().width / 8;
	unitHeight = boardSprite.getGlobalBounds().height / 8;

	widthScale = unitWidth / (pieceSprites.getGlobalBounds().width / 6);
	heightScale = unitHeight / (pieceSprites.getGlobalBounds().height / 2);


}

piece board::getPiece(int const& i) {
	return layout[i];
}

void board::update(sf::RenderWindow &window) {
	for (int i = 0; i < layout.size(); i++)
	{
		piece p = getPiece(i);
		sf::Sprite psprite = p.getSprite(pieceSprites);
		psprite.setScale(widthScale, heightScale);
		psprite.setPosition((i % 8) * unitWidth, (i / 8) * unitHeight);
		window.draw(psprite);
	}
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

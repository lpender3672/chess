#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "board.h"

board::board() {

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

piece& board::getPiece(int const& i) {
	return layout[i];
}

sf::Vector2i board::IDtoV2(int const& i) {
	return sf::Vector2i( (i % 8) * unitWidth, (i / 8) * unitHeight);
}

int board::V2toID(sf::Vector2i const& v) {
	return v.x + v.y * 8;
}

void board::update(sf::RenderWindow &window) {
	for (int i = 0; i < layout.size(); i++)
	{	
		piece p = layout[i];
		std::cout << p.name << std::endl;
		sf::Sprite pieceSprite = p.createSprite(pieceSprites);
		pieceSprite.setScale(widthScale, heightScale);
		pieceSprite.setPosition((i % 8) * unitWidth, (i / 8) * unitHeight);
		window.draw(pieceSprite);
	}
}



std::vector<int> board::getMoves(int const& i) {
	
	throw "Not implemented";
}

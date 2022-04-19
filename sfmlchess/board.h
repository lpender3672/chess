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
	board(vector<piece> boardvar, sf::Sprite _boardSprite, sf::Sprite _pieceSprites);

	piece& getPiece(int const& i);
	void update(sf::RenderWindow &window);
	
	sf::Vector2i IDtoV2(int const& i);
	int V2toID(sf::Vector2i const& v);

	vector<piece> layout;

	float widthScale;
	float heightScale;

	int unitWidth;
	int unitHeight;

	vector<int> getMoves(int const& index);


private:

	sf::Sprite boardSprite;
	sf::Sprite pieceSprites;
};
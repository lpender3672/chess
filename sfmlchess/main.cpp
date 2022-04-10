#include <SFML\Graphics.hpp>
#include <iostream>
#include <list>

#include "piece.h"
#include "board.h"

int main() {

	std::cout << piece(0).getName() << std::endl;



	int width = 800;
	int height = 800;

	sf::RenderWindow window(sf::VideoMode(width, height), "Chess");

	vector<piece> layout = vector<piece>{ piece(1), piece(2), piece(3), piece(5), piece(4), piece(3), piece(2), piece(1),
							piece(6), piece(6), piece(6), piece(6), piece(6), piece(6), piece(6), piece(6),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0), piece(0),
							piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12),
							piece(7), piece(8), piece(9), piece(10), piece(11), piece(9), piece(8), piece(7) };
	board Board;

	std::cout << Board.getPiece(4).getName() << std::endl;

	sf::Texture boardTexture;
	boardTexture.loadFromFile("sprites/board.png");
	sf::Sprite boardSprite;
	boardSprite.setTexture(boardTexture);

	sf::Texture pieceTexture;
	pieceTexture.loadFromFile("sprites/pieces.png");
	sf::Sprite pieceSprite;
	pieceSprite.setTexture(pieceTexture);

	

	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
		}
		window.clear();

		window.draw(boardSprite);

		window.display();
	}
}
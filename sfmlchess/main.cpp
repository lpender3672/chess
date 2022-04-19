#include <SFML\Graphics.hpp>
#include <iostream>
#include <list>

#include "piece.h"
#include "board.h"

int main() {


	vector<piece> layout ={ piece( 5), piece( 4), piece( 3), piece( 2), piece( 1), piece( 3), piece( 4), piece( 5),
							piece( 6), piece( 6), piece( 6), piece( 6), piece( 6), piece( 6), piece( 6), piece( 6),
							piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0),
							piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0),
							piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0),
							piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0), piece( 0),
							piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12), piece(12),
							piece(11), piece(10), piece( 9), piece( 7), piece( 8), piece( 9), piece(10), piece(11) };


	sf::Texture boardTexture;
	boardTexture.loadFromFile("sprites/board.png");
	sf::Sprite boardSprite;
	boardSprite.setTexture(boardTexture);
	boardSprite.setScale(2, 2);

	sf::Texture pieceTextures;
	pieceTextures.loadFromFile("sprites/pieces.png");
	sf::Sprite pieceSprites;
	pieceSprites.setTexture(pieceTextures);

	board Board = board(layout, boardSprite, pieceSprites);

	int width = boardSprite.getGlobalBounds().width;
	int height = boardSprite.getGlobalBounds().height;
	sf::RenderWindow window(sf::VideoMode(width, height), "Chess");
	
	static bool lock_click;

	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}

			if (event.type == sf::Event::MouseMoved) {

			}

			if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Left && !lock_click) {

					sf::Vector2i pos = sf::Vector2i(event.mouseButton.x / Board.unitWidth, event.mouseButton.y / Board.unitHeight);

					int index = Board.V2toID(pos);

					piece p = Board.getPiece(index);
					
					if (p.name != "none") {
						std::cout << "clicked " << p.colour << " " << p.name << std::endl;
					}

					lock_click = true;
				}
			}

			if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					lock_click = false;
				}
			}
		}
		window.clear();

		window.draw(boardSprite);

		Board.update(window);

		window.display();
	}
}
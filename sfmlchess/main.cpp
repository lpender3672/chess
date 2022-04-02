#include <SFML\Graphics.hpp>
#include <iostream>
#include <list>


/* move logic:

def vectorflipper(vector2_mov,vector2_pos, infinite = False):
  unit_vector2_identity_list = [[1, 1], [-1, 1], [1, -1], [-1, -1]]
  repeat = 1
  if infinite:
	repeat = 8
  moves = []
  for vector2 in unit_vector2_identity_list:
	move_plane = []
	for i in range(repeat):
	  x = (i + vector2_mov[0]) * vector2[0] + vector2_pos[0]
	  y = (i + vector2_mov[1]) * vector2[1] + vector2_pos[1]
	  move_plane.append([x,y])
	moves.append(move_plane)
  del move_plane, x, y
  return moves
  */

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




class board {
public:
	int boardvar[8][8] = {
	  { 1, 2, 3, 5, 4, 3, 2, 1},
	  { 6, 6, 6, 6, 6, 6, 6, 6},
	  { 0, 0, 0, 0, 0, 0, 0, 0},
	  { 0, 0, 0, 0, 0, 0, 0, 0},
	  { 0, 0, 0, 0, 0, 0, 0, 0}, 
	  { 0, 0, 0, 0, 0, 0, 0, 0},
	  {12,12,12,12,12,12,12,12},
	  { 7, 8, 9,10,11, 9, 8, 7} };

	std::map<int, std::map<std::string, std::string>> map = getmap();

	std::string nameboard(std::string identifier) {
		

		std::string nameboard[8][8];

		
		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 8; x++)
			{
				map[boardvar[y][x]][identifier];
			}
		}
		

		return "not done";
	}


private:

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
	




};  






int main() {

	
	board board;
	
	std::list<std::list<std::list<int>>> moves;
	int posv2[2] = { 4, 4 };
	int movv2[2] = { 1, 1 };
	moves = vectorflipper( movv2, posv2, true);

	
	std::cout << std::next(moves.begin(), 0);

	int width = 800;
	int height = 800;

	sf::RenderWindow window(sf::VideoMode(width, height), "Chess");



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
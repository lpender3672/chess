#pragma once

#include <iostream>
#include <list>
#include <map>
#include <SFML\Graphics.hpp>

using namespace std;


class piece {
public:

	piece();
	piece(int const& id);

	int getID();

	std::string name;
	std::string colour;

	sf::Sprite getSprite(sf::Sprite const& boardSprite);

private:

	int id;
	

};
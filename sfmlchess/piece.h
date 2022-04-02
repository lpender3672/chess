#pragma once

#include <iostream>
#include <list>
#include <map>

using namespace std;


class piece {
public:



	piece();
	piece(int const& id);

	int getID();
	std::string getName();

private:

	int id;
	std::string name;



};
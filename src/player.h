#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED


#include "game.h"

#include <iostream>
using namespace std;

class Game;

class Player{
public:
    Game * m_game;
//    ParRap rng;

    Player(Game * game);

    void playRandomMove();


};


#endif // PLAYER_H_INCLUDED

#include "player.h"

Player::Player(Game * game) : m_game(game){};

void Player::playRandomMove(){
    const auto& moves = m_game->availMoves();
    if(moves.size() > 0){
        auto m = moves[0];

        m_game->clickDiamond(m.first);
        m_game->clickDiamond(m.second);
        cout << "MOVED :  " << m.first.x() << "  " << m.first.y() << " --> " << m.second.x() << "  " << m.second.y() <<  endl;
    }
}

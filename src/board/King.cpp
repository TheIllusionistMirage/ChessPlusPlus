#include "King.hpp"

#include <iostream>
#include <initializer_list>

namespace chesspp
{
    namespace board
    {
        King::King(Board &b, Position_t const &pos, Suit const &s)
        : Piece(b, pos, s)
        {
        }

        void King::calcTrajectory()
        {
            std::clog << "King@" << pos << "->calcTrajectory()" << std::endl;

            //Kings can move one space in all eight directions

            for(Direction d : {Direction::North
                              ,Direction::NorthEast
                              ,Direction::East
                              ,Direction::SouthEast
                              ,Direction::South
                              ,Direction::SouthWest
                              ,Direction::West
                              ,Direction::NorthWest})
            {
                Position_t t = Position_t(pos).move(d);
                addTrajectory(t);
                addCapturing(t);
            }
        }

        King::moveAnimation(Position_t const &from, Position_t const &to)
        {
            //
        }
    }
}

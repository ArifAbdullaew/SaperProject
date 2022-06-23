#include <doctest.h>
#include "src.h"

TEST_CASE("Tesing game over state.")
{
    GameOverState go(true,2);
            CHECK(go._Status == true);
}

TEST_CASE("Checking inputs.")
{
    using namespace alone::input;
            REQUIRE((preLmb == false and nowLmb == false));
            REQUIRE((preRmb == false and nowRmb == false));
            REQUIRE(isClickedLeftButton() == false);
            REQUIRE(isClickedRightButton() == false);
}

TEST_CASE("Testing difficulty_t.")
{
    difficulty_t dif;
    dif.bombs = 2;
            CHECK(dif.bombs != 0);
}

TEST_CASE("Testing method has_bombs.")
{
    Map m;
            REQUIRE(m._HasBomb(10, 10) == false);
}

TEST_CASE("Testing GameMap pointer.")
{
    GameState g(2);
            REQUIRE(g._GameMap == nullptr);
    g.onDelete();
            REQUIRE(g._GameMap == nullptr);
}
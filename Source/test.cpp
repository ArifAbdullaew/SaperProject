#include <doctest.h>
#include "src.cpp"

TEST_CASE("Tesing game over state")
{
GameOverState go(true,2);
CHECK(go._Status == true);
}

TEST_CASE("Checking inputs")
{
using namespace alone::input;
REQUIRE((preLmb == false and nowLmb == false));
REQUIRE((preRmb == false and nowRmb == false));
}

TEST_CASE("Testing difficulty_t")
{
difficulty_t dif;
dif.bombs = 2;
CHECK(dif.bombs != 0);
}

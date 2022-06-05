#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <time.h>

int main() {
    srand(time(0));

    sf::RenderWindow window(sf::VideoMode(800, 800), "Minesweeper!");

    int w = 32;
    int grid[12][12];
    int sgrid[12][12];
    bool selected = false;                  //true, когда игрок уже нажал на какое-либо поле
    bool end_game = false;                  //true, когда либо подорвались на мине, либо нашли все мины. Запускает процесс окончания игры

    sf::Texture t;
    t.loadFromFile("htmlimage.png");
    sf::Sprite tiles(t);

    for (int i = 1; i <= 10; i++)
        for (int j = 1; j <= 10; j++) {
            sgrid[i][j] = 10;

            if (rand() % 5 == 0)
                grid[i][j] = 9;
            else
                grid[i][j] = 0;
        }

    for (int i = 1; i <= 10; i++)
        for (int j = 1; j <= 10; j++) {
            int n = 0;

            if (grid[i][j] == 9)
                continue;
            if (grid[i + 1][j] == 9)
                n++;
            if (grid[i][j + 1] == 9)
                n++;
            if (grid[i - 1][j] == 9)
                n++;
            if (grid[i][j - 1] == 9)
                n++;
            if (grid[i + 1][j + 1] == 9)
                n++;
            if (grid[i - 1][j - 1] == 9)
                n++;
            if (grid[i - 1][j + 1] == 9)
                n++;
            if (grid[i + 1][j - 1] == 9)
                n++;

            grid[i][j] = n;
        }

    //sf::Music music;
    //music.openFromFile("audio\\MainMusci.waw");
    //if (!music.openFromFile("audio\\MainMusci.ogg")) {
    //    std::cout << "ERROR" << std::endl;
    //}

    //music.play();

    while (window.isOpen()) {
        sf::Vector2i pos = sf::Mouse::getPosition(window);
        int x = pos.x / w;
        int y = pos.y / w;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
                if (event.key.code == sf::Mouse::Left)
                    sgrid[x][y] = grid[x][y];
                    selected = true;                                //Указываем, что игрок выбрал какое-то поле

            if (event.key.code == sf::Mouse::Right)
                sgrid[x][y] = 11;

        }

        window.clear(sf::Color::White);

        for (int i = 1; i <= 10; i++)
            for (int j = 1; j <= 10; j++) {
                if (sgrid[x][y] == 9)
                    sgrid[i][j] = grid[i][j];
                tiles.setTextureRect(sf::IntRect(sgrid[i][j] * w, 0, w, w));
                tiles.setPosition(i * w, j * w);

                window.draw(tiles);
            }

        window.display();
    }

    return 0;
}

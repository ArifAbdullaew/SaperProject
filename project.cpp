#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>


using namespace sf;
int width = 32;
int grid[12][12];
int sgrid[12][12];
bool selected = false;                    //true, когда игрок уже нажал на какое-либо поле
bool end_game = false;                    //true, когда либо подорвались на мине, либо нашли все мины. Запускает процесс окончания игры
int mines_found = 0;
int mines = 0;


int main() {
    RenderWindow window(VideoMode(400, 400), "Minesweeper!");


    srand(time(0));


    Texture t;
    t.loadFromFile("htmlimage.png");
    Sprite s(t);


    //Создание мин и их распределение на поле
    for (int i = 1; i <= 10; i++) {
        for (int j = 1; j <= 10; j++) {
            sgrid[i][j] = 10;
            if (rand() % 5 == 0) {
                grid[i][j] = 9;
                mines++;
            }
            else grid[i][j] = 0;
        }
    }

    //Подсчёт мин вокруг каждой клетки
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            int n = 0;
            if (grid[i][j] == 9) continue;
            if (grid[i + 1][j] == 9) n++;
            if (grid[i][j + 1] == 9) n++;
            if (grid[i - 1][j] == 9) n++;
            if (grid[i][j - 1] == 9) n++;
            if (grid[i + 1][j + 1] == 9) n++;
            if (grid[i - 1][j - 1] == 9) n++;
            if (grid[i + 1][j - 1] == 9) n++;
            if (grid[i - 1][j + 1] == 9) n++;
            grid[i][j] = n;
        }
    }


    //Задаёт стандартное отображение по началу игры
    for (int i = 0; i <= 10; i++) {
        for (int j = 0; j <= 10; j++) {
            sgrid[i][j] = 10;
        }
    }


    while (window.isOpen()) {

        Vector2i pos = Mouse::getPosition(window);
        int x = pos.x / width;
        int y = pos.y / width;

        Event event;

        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();

            if (event.type == Event::MouseButtonPressed) {
                if (event.key.code == Mouse::Left) {
                    sgrid[x][y] = grid[x][y];
                    selected = true;                                //Указываем, что игрок выбрал какое-то поле
                } else if (event.key.code == Mouse::Right && sgrid[x][y] != 11) {
                    sgrid[x][y] = 11;
                    if (grid[x][y] == 9 && sgrid[x][y] == 11)mines_found++;
                }            //Тег-флаг + проверка на правильность метки

                else {
                    sgrid[x][y] = 10;
                    if (grid[x][y] == 9 && selected == false)
                        mines_found--;                //Если кликнуть на флаг правой кнопкой мыши, флаг будет снят (если при этом это был правильный флажок, количество обнаруженных мин тоже уменьшаем)
                }
            }
        }


        window.clear(Color::White);


        //Проверка на мину
        if (selected == true) {
            if (event.key.code == Mouse::Left &&
                grid[x][y] == 9)            //Если игрок выбрал поле с миной, завершаем игру
            {
                for (int i = 0; i <= 10; i++) {
                    for (int j = 0; j <= 10; j++) {
                        sgrid[i][j] = grid[i][j];
                        end_game = true;                                    //Запускаем конец игры


                    }
                }

            }

        }

        //Проверка на то, нашёл ли игрок все мины

        if (mines_found == mines) {
            end_game = true;
        }

        //Отрисовка
        for (int i = 0; i <= 10; i++) {
            for (int j = 0; j <= 10; j++) {

                s.setTextureRect(IntRect(sgrid[i][j] * width, 0, width, width));
                s.setPosition(i * width, j * width);
                window.draw(s);

            }
        }
        selected = false;

        window.display();
    }


    //Окно Game Over!

    if (end_game == true) {
        RenderWindow end(VideoMode(360, 60), "Game Over!");                //Создаём новое окошко
        Font font;                                                        //Новый шрифт
        font.loadFromFile("C:\\Windows\\Fonts\\Calibrib.ttf");            //передаем нашему шрифту файл шрифта

        while (end.isOpen()) {
            Text text("", font, 17);                                    //Создаём два текста
            Text record("", font, 17);
            if (mines_found != mines) {
                text.setString("The game is over! You lost!");
            }                //Если нашли все мины, то победа. В противном случае поражение:)
            else text.setString("The game is over! You win!");

            record.setString("Bombs found " + std::to_string(mines_found) + " from " + std::to_string(mines) +
                             "!");                        //Записываем в текст, сколько мин из возможных было найдено

            text.setFillColor(Color::Black);
            text.setPosition(Vector2f(10, 5));

            record.setFillColor(Color::Black);
            record.setPosition(Vector2f(10, 25));

            end.draw(text);
            end.draw(record);
            Event a;
            while (end.pollEvent(a)) {
                if (a.type == Event::Closed) end.close();
            }
            end.clear(Color::White);
            end.draw(text);
            end.draw(record);
            end.display();


        }

    }
    return 0;
}

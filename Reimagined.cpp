//std
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <random>
#include <queue>
#include <functional>
#include <iostream>
#include <memory>

//sfml
#include <SFML/Graphics.hpp>

#define DEBUG_MODE 0

sf::RenderWindow window(sf::VideoMode(450, 800), "Minesweeper");
sf::Font font;

namespace alone {
    //контейнер для управления текстурками
    class TextureManager {
    public:
        void load(std::string config_name) {
            std::ifstream file(config_name);

            while (!file.eof()) {
                std::string temp;
                file >> temp;

                sf::Texture texture;
                texture.loadFromFile("assets/textures/" + temp);

                _Content.emplace(temp, std::move(texture));
            }
        }

        sf::Texture& operator[](std::string key) {
            return _Content.at(key);
        }

    private:
        std::unordered_map <std::string, sf::Texture> _Content;
    };

    //базовое состояние игры, от него насследуются все остальные
    class State : public sf::Drawable {
        friend class StateMachine;
    public:
        enum Status {
            OnCreate,
            Active,
            OnDelete
        };

    protected:
        //не стали использовать делту, ибо это бесполезно в сапёре
        virtual void update() = 0;

        virtual void onCreate() = 0;
        virtual void onDelete() = 0;

    private:
        Status _Status;
    };

    class StateMachine {
    public:
        void insert(std::string key, std::shared_ptr <State> value) {
            value->_Status = State::OnCreate;
            _Content.emplace(key, value);
        }
        void erase(std::string key) {
            auto it = _Content.find(key);
            if (it != _Content.end())
                it->second->_Status = State::OnDelete;
        }

        void update() {
           //была проблема с контейнером, нельзя во время иттерации элементы удалять
			//поэтому мы сделали очередь для этих элементов
            std::queue <std::string> onRemove;
            for (auto& it : _Content) {
                switch (it.second->_Status) {
                    case State::OnCreate:
                        it.second->onCreate();
                        it.second->_Status = State::Active;
                        break;
                    case State::Active:
                        it.second->update();
                        it.second->draw(window, sf::RenderStates::Default);
                        break;
                    case State::OnDelete:
                        it.second->onDelete();
                        onRemove.push(it.first);
                        break;
                }
            }

            while (!onRemove.empty()) {
                _Content.erase(onRemove.front());
                onRemove.pop();
            }
        }

    private:
        std::unordered_map <std::string, std::shared_ptr <State>> _Content;
    };
}

//просто большой костыль, не обращайте внимания
namespace alone::input {
    bool preLmb = false, nowLmb = false;
    bool preRmb = false, nowRmb = false;

    void update() {
        preLmb = nowLmb;
        preRmb = nowRmb;
        nowLmb = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        nowRmb = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    }

    bool isClickedLeftButton() {
        return preLmb && !nowLmb;
    }
    bool isClickedRightButton() {
        return preRmb && !nowRmb;
    }
}

//just a crutch for fast naming
template <class _T>
using Matrix = std::vector <std::vector <_T>>;

//это для удобной нумерации текстурок
enum class Type {
    Number1 = 0,
    Number2 = 1,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    None,
    Unknown,
    Flag,
    NoBomb,
    NoneQuiestion,
    UnknownQuestion,
    Bomb,
    RedBomb
};

//crutch
struct difficulty_t {
    std::string name;
    size_t bombs;
    size_t size;
};

std::array <difficulty_t, 3> difficulties;
alone::TextureManager textures;
alone::StateMachine states;

//it's unnecessary
//size_t hash(std::pair <size_t, size_t> val) {
//	return (val.first << 1) ^ (val.second >> 1);
//}

class Map {
    friend class GameState;
public:
    void resize(size_t level) {
        auto& d = difficulties[level];

        _Content.resize(d.size);
        for (auto& it : _Content)
            it.resize(d.size, { 'n', Type::None});
    }
    
    
    //генерация карты, включая рандомное заполнение
    
    void generate(size_t level, sf::Vector2u point) {
        auto& d = difficulties[level];
        _Bombs = d.bombs;

        std::set <size_t> unfilled;
        for (size_t i = 0; i != d.size * d.size; i++)
            unfilled.emplace(i);

        unfilled.erase(point.x + point.y * d.size);

        std::random_device rd;

        //заполнение бомб
        for (size_t i = 0; i != _Bombs; i++) {
            std::uniform_int_distribution <size_t> dist(0, unfilled.size() - 1);
            size_t pos = dist(rd);
            //std::cout << pos << ' ';

            auto it = unfilled.begin();
            std::advance(it, pos);
            size_t point = *it;
            _Content[point % d.size][point / d.size] = { 'n', Type::Bomb};
            unfilled.erase(point);
        }
        //std::cout << '\n';

        //заполнение чиселок вокруг бомб
        for (size_t i = 0; i != _Content.size(); i++) {
            for (size_t j = 0; j != _Content[i].size(); j++) {
                if (_Content[i][j].second == Type::Bomb)
                    continue;

                size_t value = _DetectAround(i, j);
                if (value != 0)
                    _Content[i][j].second = (Type)(value - 1);
            }
        }
    }

private:
    //n - unknown, r - revealed, f - flag
    Matrix <std::pair <char, Type>> _Content;
    size_t _Bombs;

    bool _HasBomb(size_t x, size_t y) {
        if (x < 0 || x >= _Content.size() || y < 0 || y >= _Content.size())
            return false;
        return _Content[x][y].second == Type::Bomb;
    };
    size_t _DetectAround(size_t x, size_t y) {
        return _HasBomb(x - 1, y - 1) + _HasBomb(x, y - 1) + _HasBomb(x + 1, y - 1) +
               _HasBomb(x - 1, y) + _HasBomb(x + 1, y) +
               _HasBomb(x - 1, y + 1) + _HasBomb(x, y + 1) + _HasBomb(x + 1, y + 1);
    };
    
    //рукурсивный алгоритм, чтобы открыть тайлы вокруг нажатого
	//получается рекурсия только в том случае, если игрок нажимает по пустому тайлу
    void _OpenTiles(int x, int y) {
        if (y >= _Content.size() || y < 0 || x >= _Content.size() || x < 0)
            return;

        if (_Content[x][y].first == 'r' || _Content[x][y].second != Type::None) {
            _Content[x][y].first = 'r';
            return;
        }

        _Content[x][y].first = 'r';

        _OpenTiles(x - 1, y - 1);
        _OpenTiles(x, y - 1);
        _OpenTiles(x + 1, y - 1);

        _OpenTiles(x - 1, y);
        _OpenTiles(x + 1, y);

        _OpenTiles(x - 1, y + 1);
        _OpenTiles(x, y + 1);
        _OpenTiles(x + 1, y + 1);
    }
};

class MenuState : public alone::State {
public:
    MenuState();

private:
    void update() override {
        auto mouse = sf::Mouse::getPosition(window);

        for (size_t i = 0; i != _Buttons.size(); i++) {
            auto bounds = _Buttons[i].getGlobalBounds();
            //std::cout << mouse.x << ' ' << mouse.y << ' ' << alone::input::isClicked() << '\n';
            if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
                _Params[i].second();
            }
        }
    }

    void onCreate() override {
        window.setSize(sf::Vector2u(450, 800));
        _Buttons.resize(_Params.size());

        for (size_t i = 0; i != _Buttons.size(); i++) {
            auto& text = _Buttons[i];
            text.setString(_Params[i].first);
            text.setFont(font);
            text.setPosition(50, 100 + i * 50);

            auto bounds = text.getGlobalBounds();
            //std::cout << bounds.left << ' ' << bounds.top << ' ' << bounds.width << ' ' << bounds.height << '\n';
        }
    }
    void onDelete() override {}

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override {
        for (const auto& it : _Buttons)
            target.draw(it, states);
    }

private:
    std::vector <sf::Text> _Buttons;
    std::array <std::pair <std::string, std::function <void()>>, 4> _Params;
};

class GameOverState : public alone::State {
public:
    GameOverState(bool status, size_t bombsFound) {
        _Status = status;
        _BombsFound = bombsFound;
    }

private:
    sf::Text _Label, _Exit;
    //1 = win, 0 = lose
    bool _Status;
    size_t _BombsFound;

    void update() override {
        auto mouse = sf::Mouse::getPosition(window);
        auto bounds = _Exit.getGlobalBounds();

        if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
            states.erase("over");
            states.insert("menu", std::shared_ptr <State>(new MenuState()));
        }
    }

    //такое чувство, что в qt попал
	//там тоже объявление интерфейса внутри кода
    
    void onCreate() override {
        _Label.setCharacterSize(42);
        _Exit.setCharacterSize(42);
        std::string text;
        text = "You ";
        if (_Status)
            text += "win";
        else
            text += "lose";

        text += "\n\nYou found " + std::to_string(_BombsFound) + " bombs!";

        _Label.setFont(font);
        _Exit.setFont(font);

        _Label.setString(text);
        _Exit.setString("Exit");

        _Label.setFillColor(sf::Color::White);
        _Exit.setFillColor(sf::Color::White);

        _Label.setOutlineColor(sf::Color::White);
        _Exit.setOutlineColor(sf::Color::White);

        _Label.setPosition(20, 20);

        auto labelBounds = _Label.getGlobalBounds();

        _Exit.setPosition(20, labelBounds.height + 40);

        auto exitBounds = _Exit.getGlobalBounds();

        window.setSize(sf::Vector2u(labelBounds.width + 80, labelBounds.height + 80 + exitBounds.height));
    }
    void onDelete() override {}

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override {
        target.draw(_Label, states);
        target.draw(_Exit, states);
    }
};

class GameState : public alone::State {
public:
    GameState(size_t level) {
        _Level = level;
    }

private:
    std::unique_ptr <Map> _GameMap;
    const size_t _InterfaceOffset = 100;
    size_t _Flags = 0;
    sf::VertexArray _RenderRegion = sf::VertexArray(sf::Quads);
    sf::Texture* _Atlas = nullptr;
    size_t _Level;
    size_t _Revealed = 0;
    sf::Clock _Clock;
    sf::Text _RemainedLabel, _TimerLabel;
    //a - active, w - win, l - lose
    char _GameStatus = 'a';

    void update() override {
        size_t edge_size = _GameMap->_Content.size();
        _RenderRegion.resize(4 * edge_size * edge_size);

        auto& map = _GameMap->_Content;

        //update timer
        auto time = _Clock.getElapsedTime();
        size_t seconds = time.asSeconds();
        _TimerLabel.setString(std::to_string(seconds / 60) + ':' + std::to_string(seconds % 60));

        //part for clicking on map
        auto mouse = sf::Mouse::getPosition(window);
        bool contains = mouse.x >= 0 && mouse.x <= edge_size * 32 && mouse.y >= _InterfaceOffset && mouse.y <= edge_size * 32 + _InterfaceOffset;
        if (contains) {
            auto point = sf::Vector2u(mouse.x / 32, (mouse.y - _InterfaceOffset) / 32);
            if (alone::input::isClickedLeftButton()) {
                if (_Revealed == 0) {
                    _GameMap->generate(_Level, point);
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(_GameMap->_Bombs));
                    _Revealed++;

                    if (map[point.x][point.y].second == Type::None)
                        _GameMap->_OpenTiles(point.x, point.y);
                    else
                        map[point.x][point.y].first = 'r';
                } else {
                    if (map[point.x][point.y].first == 'n') {
                        if (map[point.x][point.y].second == Type::None)
                            _GameMap->_OpenTiles(point.x, point.y);
                        else
                            map[point.x][point.y].first = 'r';

                        if (map[point.x][point.y].second == Type::Bomb)
                            _GameStatus = 'l';

                        _Revealed++;
                    }
                }
            } else if (alone::input::isClickedRightButton()) {
                if (map[point.x][point.y].first == 'f') {
                    map[point.x][point.y].first = 'n';
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(++_GameMap->_Bombs));

                    if (map[point.x][point.y].second == Type::Bomb)
                        _Flags--;
                } else if (map[point.x][point.y].first == 'n') {
                    map[point.x][point.y].first = 'f';
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(--_GameMap->_Bombs));

                    if (map[point.x][point.y].second == Type::Bomb)
                        _Flags++;

                    if (_Flags == difficulties[_Level].bombs) {
                        _GameStatus = 'w';
                    }
                }
            }
        }

        //расчёт вершин для карты
        for (size_t i = 0; i != map.size(); i++) {
            for (size_t j = 0; j != map[i].size(); j++) {
                auto& top_lhs = _RenderRegion[(i + j * map.size()) * 4];
                auto& top_rhs = _RenderRegion[(i + j * map.size()) * 4 + 1];
                auto& bot_rhs = _RenderRegion[(i + j * map.size()) * 4 + 2];
                auto& bot_lhs = _RenderRegion[(i + j * map.size()) * 4 + 3];

                size_t id = 0;
                if (DEBUG_MODE || map[i][j].first == 'r')
                    id = (size_t)map[i][j].second;
                else if (map[i][j].first == 'f')
                    id = (size_t)Type::Flag;
                else
                    id = (size_t)Type::Unknown;


                size_t idx = id % 4;
                size_t idy = id / 4;

                top_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32);
                top_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32);
                bot_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32 + 32);
                bot_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32 + 32);

                top_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f);
                top_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f);
                bot_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f + 32);
                bot_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f + 32);
            }
        }
        		//костыли

        if (_GameStatus != 'a') {
            states.erase("game");
            states.insert("over", std::shared_ptr <State>(new GameOverState(_GameStatus == 'w', _Flags)));
        }
    }

    void onCreate() override {
        _Clock.restart();

        _GameMap.reset(new Map());
        _GameMap->resize(_Level);

        _Revealed = 0;

        size_t edge_size = _GameMap->_Content.size();
        window.setSize(sf::Vector2u(edge_size * 32, edge_size * 32 + _InterfaceOffset));

        _Atlas = &textures["minesweeper.png"];

        _RemainedLabel.setFont(font);
        _TimerLabel.setFont(font);

        _RemainedLabel.setFillColor(sf::Color::White);
        _TimerLabel.setFillColor(sf::Color::White);

        _RemainedLabel.setOutlineColor(sf::Color::White);
        _TimerLabel.setOutlineColor(sf::Color::White);

        _RemainedLabel.setPosition(20, 15);
        _TimerLabel.setPosition(20, 50);

        if (_Level == 0) {
            _RemainedLabel.setCharacterSize(24);
            _TimerLabel.setCharacterSize(24);
        }
    }
    void onDelete() override {
        _GameMap.reset(nullptr);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override {
        states.texture = _Atlas;
        target.draw(_RenderRegion, states);

        target.draw(_RemainedLabel, states);
        target.draw(_TimerLabel, states);
    }
};

//тоже костыли, но мне пофиг
//нужно было создавать .cpp файлы

MenuState::MenuState() {
    _Params = {
            std::make_pair(std::string("Easy"), []() {
                states.insert("game", std::shared_ptr <alone::State>(new GameState(0)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Medium"), []() {
                states.insert("game", std::shared_ptr <alone::State>(new GameState(1)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Hard"), []() {
                states.insert("game", std::shared_ptr <alone::State>(new GameState(2)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Exit"), []() {
                window.close();
            })
    };
}

//usually I add one big context struct, where defined every important controller, but now it's enough
void init() {
    textures.load("assets/textures/include.txt");

    difficulties[0] = { "Easy", 10, 8 };
    difficulties[1] = { "Medium", 20, 10 };
    difficulties[2] = { "Hard", 70, 20 };

    font.loadFromFile("assets/font.ttf");

    states.insert("menu", std::shared_ptr <alone::State>(new MenuState()));
}

int main() {
    init();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    break;
            }
        }

        window.clear();

        alone::input::update();
        states.update();

        window.display();
    }

    return 0;
}

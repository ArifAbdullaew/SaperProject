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
        void load(std::string config_name);
        sf::Texture& operator[](std::string key);
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
        void insert(std::string key, std::shared_ptr <State> value);
        void erase(std::string key);
        void update();
    private:
        std::unordered_map <std::string, std::shared_ptr <State>> _Content;
    };
}

namespace alone::input {
    bool preLmb = false, nowLmb = false;
    bool preRmb = false, nowRmb = false;

    void update();
    bool isClickedLeftButton();
    bool isClickedRightButton();
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

class Map {
    friend class GameState;
public:
    void resize(size_t level);

    //генерация карты, включая рандомное заполнение
    void generate(size_t level, sf::Vector2u point);

    //n - unknown, r - revealed, f - flag
    Matrix <std::pair <char, Type>> _Content;
    size_t _Bombs;

    bool _HasBomb(size_t x, size_t y);

    size_t _DetectAround(size_t x, size_t y);

    //рукурсивный алгоритм, чтобы открыть тайлы вокруг нажатого
    //получается рекурсия только в том случае, если игрок нажимает по пустому тайлу
    void _OpenTiles(int x, int y);
};

class MenuState : public alone::State {
public:
    MenuState();
private:
    void update() override;

    void onCreate() override;

    void onDelete() override;

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override;

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

    sf::Text _Label, _Exit;
    //1 = win, 0 = lose
    bool _Status;
    size_t _BombsFound;

    void update() override;

    //такое чувство, что в qt попал
    //там тоже объявление интерфейса внутри кода

    void onCreate() override;

    void onDelete() override;

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override;
};

class GameState : public alone::State {
public:
    GameState(size_t level) {
        _Level = level;
    }
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

    void update() override;

    void onCreate() override;

    void onDelete() override;

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override;
};

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
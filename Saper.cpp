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
#include <SFML/Audio.hpp>

#define DEBUG_MODE 0

sf::RenderWindow window(sf::VideoMode(450, 800), "Minesweeper");
sf::Font font;

namespace alone {
    /**
     * контейнер для управления текстурками
     */
    class TextureManager {
    public:
        /**
         * подгрузка текстурок из конфига
         * @param config_name
         */
        void load(std::string config_name) {
            std::ifstream file(config_name);

            while (!file.eof()) {
                std::string temp;
                file >> temp;
                /**
                 * загружается только из этой папки, но сам файл может быть с кастомным именем
                 */
                sf::Texture texture;
                texture.loadFromFile("assets/textures/" + temp);
                /**
                 * Вставка в контейнер
                 */
                _Content.emplace(temp, std::move(texture));
            }
        }

        /**
         * перегрузка оператора индексирования для более красивого кода
         * @param key
         * @return
         */
        sf::Texture &operator[](std::string key) {
            return _Content.at(key);
        }

    private:
        /**
         * unordered_map работае быстрее обычной map
         */
        std::unordered_map<std::string, sf::Texture> _Content;
    };

    /**
     *  базовое состояние игры, от него насследуются все остальные
	    отвечает за всё в своём процессе
	    наследуется от Drawable, чтобы было меньше кода писать
     */
    class State : public sf::Drawable {
        friend class StateMachine;

    public:
        /**
         * статусы для более удобного использования состояний игры
         */
        enum Status {
            OnCreate,
            Active,
            OnDelete
        };

    protected:
        /**
         * не стал использовать делту, ибо это бесполезно в сапёре
         */
        virtual void update() = 0;

        /**
         * базовый метод, которое вызывают при создании состояния
         */
        virtual void onCreate() = 0;

        /**
         * а этот при удалении
         */
        virtual void onDelete() = 0;

    private:
        Status _Status;
    };

    /**
     * Машина состояний, являющаяся контейнром состояний и их инвокером
	    Также отвечает за отрисовку
     */
    class StateMachine {
    public:
        /**
         * Вставка нового состояния с определённым ключом и заранее созданым состоянием
		    factory для состояний является лишней
         * @param key
         * @param value
         */
        void insert(std::string key, std::shared_ptr<State> value) {
            value->_Status = State::OnCreate;
            _Content.emplace(key, value);
        }

        /**
         * Убирает состояние из контейнера, но не сразу же, а после следующего обновления
         * @param key
         */
        void erase(std::string key) {
            auto it = _Content.find(key);
            if (it != _Content.end())
                it->second->_Status = State::OnDelete;
        }

        void update() {
            /**
             * была проблема с контейнером, нельзя во время иттерации элементы удалять
                поэтому мы сделали очередь для этих элементов
            */
            std::queue<std::string> onRemove;
            for (auto &it: _Content) {
                switch (it.second->_Status) {
                    case State::OnCreate:
                        it.second->onCreate();
                        it.second->_Status = State::Active;
                        break;

                        /**
                         * основной статус, в котором проводит время состояние игры
                         *
                         */
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
            /**
             * Непосредственное удаление элементов, которые были запрошены для этого во время работы процесса
             *
             */
            while (!onRemove.empty()) {
                _Content.erase(onRemove.front());
                onRemove.pop();
            }
        }

    private:
        std::unordered_map<std::string, std::shared_ptr<State>> _Content;
    };
}

//просто большой костыль, не обращайте внимания
namespace alone::input {
    /**
     * это для левой кнопки мыши
     */
    bool preLmb = false, nowLmb = false;

    /**
     * это для правой
     */
    bool preRmb = false, nowRmb = false;

    /**
     *  pre - состояние нажатия во время прошлого обнолвения
	    now - во время текущего

	    обновляет текущее состояие нажатия обеих клавиш
     */

    void update() {
        preLmb = nowLmb;
        preRmb = nowRmb;
        nowLmb = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        nowRmb = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    }

    /**
     *  запрос состояния нажатия левой кнопки мыши
	    выдаёт true только если кнопка была только что поднята
     * @return
     */
    bool isClickedLeftButton() {
        return preLmb && !nowLmb;
    }

    /**
     * то же самое, но для правой
     * @return
     */
    bool isClickedRightButton() {
        return preRmb && !nowRmb;
    }
}

/**
 * упрощение нейминга
 */
template<class _T>
using Matrix = std::vector<std::vector<_T>>;


/**
 * это для удобной нумерации текстурок
 */
enum class Type {
    /**
     * кол-во бомб от 1 до 8
     */
    Number1 = 0,
    Number2 = 1,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,

    /**
     * бомб нет
     */
    None,

    /**
     * неизвестно что тут
     */
    Unknown,

    /**
     * флажок, если бомба тут есть
     */
    Flag,

    /**
     * бомбы нет
     */
    NoBomb,

    /**
     * нет ничего
     */
    NoneQuiestion,

    /**
     * неизвестно и вопрос
     */
    UnknownQuestion,

    /**
     * тут бомба
     */
    Bomb,

    /**
     * бомба взорвалась
     */
    RedBomb

};

/**
 * класс для удобного хранения уровня сложности
 */
struct difficulty_t {
    /**
     * имя уровня сложности
     */
    std::string name;

    /**
     * количество бомб на карте
     */
    size_t bombs;

    /**
     * размер грани карты, карта может быть только квадратная
     */
    size_t size;
};

/**
 * набор уровней сложности, свой выдавать нельзя
 */
std::array<difficulty_t, 3> difficulties;

/**
 * менеджер текстур, это просто инициализация
 */
alone::TextureManager textures;

/**
 * то же самое, но для состояний
 */
alone::StateMachine states;

/**
 * класс карты игры
 */
class Map {
    /**
     * позволяет работать с картой игры не напрямую
     */
    friend class GameState;

public:
    /**
     *  изменение размера карты игры в зависимости от уровня сложности
	    который берётся из std::array <difficulty_t, 3> difficulties
     * @param level
     */
    void resize(size_t level) {
        /**
         * собственно и взятие уровня сложности
         */
        auto &d = difficulties[level];

        /**
         * изменение ширины поля карты
         */
        _Content.resize(d.size);
        for (auto &it: _Content)
            /**
             * изменение высота поля, для каждой колонки в отдельности
             */
            it.resize(d.size, {'n', Type::None});
    }


    /**
     *  генерация карты, включая рандомное заполнение
	    то же берёт заранее заготовленный уровень сложности из std::array <difficulty_t, 3> difficulties
	    point - это точка, в которую нажал игрок
	    сделано, чтобы игрок не мог проиграть с первого нажатия
     * @param level
     * @param point это точка, в которую нажал игрок
     */

    void generate(size_t level, sf::Vector2u point) {
        auto &d = difficulties[level];
        _Bombs = d.bombs;

        /**
         *  переменная для незаполненных клеток карты
		    достаточно много весит, вначале 'размер_грани ^ 2 - 1'
         */
        std::set<size_t> unfilled;

        /**
         *  заполяем все точки с карты так, что элемент с индексом 0 - это левый верхний
		    а с индексом 10 при ширине в 8 тайлов - это элемент с 'x = 1' и 'y = 2'
         */
        for (size_t i = 0; i != d.size * d.size; i++)
            unfilled.emplace(i);

        /**
         * убираем ту самую точку, в которую вначале нажал игрок
         */
        unfilled.erase(point.x + point.y * d.size);

        /**
         * рандомный генератор
         */
        std::random_device rd;

        /**
         * заполнение бомб
         */
        for (size_t i = 0; i != _Bombs; i++) {

            /**
             * генерация рандомного индекса числа из сета
             */
            std::uniform_int_distribution<size_t> dist(0, unfilled.size() - 1);
            size_t pos = dist(rd);

            /**
             *  тупо огромный костыль для работы с std::set, более элегантного решения мы не нашли
             */


            auto it = unfilled.begin();
            std::advance(it, pos);

            /**
             *  point - это точка, в которую поместят бомбу
			    берёт значение итератора из std::set <size_t> unfilled
             */
            size_t point = *it;
            _Content[point % d.size][point / d.size] = {'n', Type::Bomb};

            /**
             * удаляет это точку из unfilled, тк она уже заполнена
             */
            unfilled.erase(point);
        }
        //std::cout << '\n';

        /**
         * заполнение чисел вокруг бомб
         */
        for (size_t i = 0; i != _Content.size(); i++) {
            for (size_t j = 0; j != _Content[i].size(); j++) {
                /**
                 * пропускаем, если здесь есть бомба
                 */
                if (_Content[i][j].second == Type::Bomb)
                    continue;

                /**
                 * проверяет  соседние клетки
                 */
                size_t value = _DetectAround(i, j);

                /**
                 * изменяет значение кол-ва бомб, если их больше 0
                 */
                if (value != 0)
                    _Content[i][j].second = (Type) (value - 1);
            }
        }
    }

private:
    /**
     *  костыль из использования char'а как состояния для отрисовки
	    n - unknown, r - revealed, f - flag
     */
    Matrix<std::pair<char, Type>> _Content;

    /**
     * кол-во бомб на карте
     */
    size_t _Bombs;

    /**
     *  проверяет, есть ли бомба по заданному индексу
	    если выходит индекс за пределы карты, то возвращает false
     * @param x
     * @param y
     * @return
     */
    bool _HasBomb(size_t x, size_t y) {
        if (x < 0 || x >= _Content.size() || y < 0 || y >= _Content.size())
            return false;
        return _Content[x][y].second == Type::Bomb;
    };

    /**
     * проверяет все клетки сверху, снизу, по бокам и по диагонали
     * @param x
     * @param y
     * @return
     */
    size_t _DetectAround(size_t x, size_t y) {
        return _HasBomb(x - 1, y - 1) + _HasBomb(x, y - 1) + _HasBomb(x + 1, y - 1) +
               _HasBomb(x - 1, y) + _HasBomb(x + 1, y) +
               _HasBomb(x - 1, y + 1) + _HasBomb(x, y + 1) + _HasBomb(x + 1, y + 1);
    };

    /**
     *  рукурсивный алгоритм, чтобы открыть тайлы вокруг нажатого
	    получается рекурсия только в том случае, если игрок нажимает по пустому тайлу
     * @param x
     * @param y
     */
    void _OpenTiles(int x, int y) {
        if (y >= _Content.size() || y < 0 || x >= _Content.size() || x < 0)
            return;

        if (_Content[x][y].first == 'r' || _Content[x][y].second != Type::None) {
            /**
             * ставит статус revealed - проверено
             */
            _Content[x][y].first = 'r';
            return;
        }

        /**
         * ставит статус revealed - проверено
         */
        _Content[x][y].first = 'r';

        /**
         * проверка верхних
         */
        _OpenTiles(x - 1, y - 1);
        _OpenTiles(x, y - 1);
        _OpenTiles(x + 1, y - 1);

        /**
         * по бокам
         */
        _OpenTiles(x - 1, y);
        _OpenTiles(x + 1, y);

        /**
         * нижних тайлов
         */
        _OpenTiles(x - 1, y + 1);
        _OpenTiles(x, y + 1);
        _OpenTiles(x + 1, y + 1);
    }
};

/**
 * состояние для меню, чтобы было проще ей управлять
 */
class MenuState : public alone::State {
public:
    MenuState();

private:

    void update() override {
        /**
         * получаем координаты мышки на экране в зависимости от положения самого окна
         */
        auto mouse = sf::Mouse::getPosition(window);

        /**
             * Отдельный массив для кнопек в меню, чтобы легче было их опознавать
             */
        for (size_t i = 0; i != _Buttons.size(); i++) {

            /**
             * получаем глобальные координаты кнопочки
             */
            auto bounds = _Buttons[i].getGlobalBounds();
            //std::cout << mouse.x << ' ' << mouse.y << ' ' << alone::input::isClicked() << '\n';
            if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
                _Params[i].second();
            }
        }
    }

    /**
     * создаёт интерфейс для меню и объявляет переменные
     */
    void onCreate() override {
        /**
         * устанавливает размер экрана игры
         */
        window.setSize(sf::Vector2u(450, 800));

        /**
         * изменяет размер динамического массива с кнопками по количеству параметров
         */
        _Buttons.resize(_Params.size());


        for (size_t i = 0; i != _Buttons.size(); i++) {
            auto &text = _Buttons[i];
            text.setString(_Params[i].first);
            text.setFont(font);
            text.setPosition(50, 100 + i * 50);

            auto bounds = text.getGlobalBounds();
            //std::cout << bounds.left << ' ' << bounds.top << ' ' << bounds.width << ' ' << bounds.height << '\n';
        }
    }

    void onDelete() override {}

    /**
     * отрисовка кнопочек
     * @param target
     * @param states
     */
    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override {
        for (const auto &it: _Buttons)
            target.draw(it, states);
    }

private:
    /**
     * массив с кнопками, инициализируется только во время вызова метода onCreate
     */
    std::vector<sf::Text> _Buttons;

    /**
     * заранее заготовленные параметры для кнопок, создаются в конструкторе
     */
    std::array<std::pair<std::string, std::function<void()>>, 4> _Params;
};

/**
 * состояние для окончания игры, возникает после окончания игры
 */
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

    /**
     * обновление экрана
     */
    void update() override {
        auto mouse = sf::Mouse::getPosition(window);
        auto bounds = _Exit.getGlobalBounds();

        /**
         * проверка, была ли нажата кнопка выхода из игры
         */
        if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
            /**
             * убирает среди состояний саму себя
             */
            states.erase("over");

            /**
             * и добавляет состояние меню
             */
            states.insert("menu", std::shared_ptr<State>(new MenuState()));
        }
    }

    /**
     * объявление интерфейса
     */

    void onCreate() override {
        /**
         * объявление размера шрифта для надписи о статусе игры
         */
        _Label.setCharacterSize(42);

        /**
         * и кнопки выхода из неё
         */
        _Exit.setCharacterSize(42);

        /**
         * генерация текста, который увидит игрок, если выиграл или проиграл
         */
        std::string text;
        text = "You ";
        if (_Status)
            text += "win";
        else
            text += "lose";

        /**
         * текст для количества бомб, которые нашёл игрок
         */
        text += "\n\nYou found " + std::to_string(_BombsFound) + " bombs!";

        /**
         * установка шрифта
         */
        _Label.setFont(font);
        _Exit.setFont(font);

        /**
         * установка текста
         */
        _Label.setString(text);
        _Exit.setString("Exit");

        /**
         * цвет внутри текста
         */
        _Label.setFillColor(sf::Color::White);
        _Exit.setFillColor(sf::Color::White);

        /**
         * цвет обода текста
         */
        _Label.setOutlineColor(sf::Color::White);
        _Exit.setOutlineColor(sf::Color::White);


        /**
         * установка местоположения для главной надписи о статусе выигрыша игрока
         */
        _Label.setPosition(20, 20);

        auto labelBounds = _Label.getGlobalBounds();

        /**
         * установка местоположения для кнопки выхода в главное меню в зависимости от предыдущей кнопки
         */
        _Exit.setPosition(20, labelBounds.height + 40);

        auto exitBounds = _Exit.getGlobalBounds();


        /**
         * установка размера окна в зависимости от размера надписи о статусе выигрыша игрока
         */
        window.setSize(sf::Vector2u(labelBounds.width + 80, labelBounds.height + 80 + exitBounds.height));
    }

    void onDelete() override {}

    /**
     * быстрая отрисовка заранее заготовленных кнопочек
     * @param target
     * @param states
     */
    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override {
        target.draw(_Label, states);
        target.draw(_Exit, states);
    }
};

/**
 * главное состояние, состояние игры
 */
class GameState : public alone::State {
public:
    /**
     * установка уровня сложности
     */
    GameState(size_t level) {
        _Level = level;
    }

private:
    /**
     * указатель на карту игры
     */
    std::unique_ptr<Map> _GameMap;

    /**
     * ограничиваем количество чисел
     */
    const size_t _InterfaceOffset = 100;

    /**
     * количество заполненных флагов на карте
     */
    size_t _Flags = 0;

    /**
     * вершины для отрисовки карты
     */
    sf::VertexArray _RenderRegion = sf::VertexArray(sf::Quads);

    /**
     * атлас с текстурами для быстрой и правильной отрисовки вершин у карты
     */
    sf::Texture *_Atlas = nullptr;

    /**
     * уровень сложности
     */
    size_t _Level;

    /**
     * количество открытых клеток
     */
    size_t _Revealed = 0;

    /**
     * timer
     */
    sf::Clock _Clock;

    /**
     * две надписи с прошедшим временем после начала игры и количеством оставшихся бомб
     */
    sf::Text _RemainedLabel, _TimerLabel;
    //a - active, w - win, l - lose
    char _GameStatus = 'a';

    void update() override {
        /**
         * размер грани карты
         */
        size_t edge_size = _GameMap->_Content.size();

        /**
         *  меняю размер массива вершин для карты игры
		    умножаем на 4, так как квадратная карта
         */
        _RenderRegion.resize(4 * edge_size * edge_size);


        auto &map = _GameMap->_Content;

        /**
         * update timer и вывод секунд
         */
        auto time = _Clock.getElapsedTime();
        size_t seconds = time.asSeconds();

        /**
         * вывод таймера
         */
        _TimerLabel.setString(std::to_string(seconds / 60) + ':' + std::to_string(seconds % 60));

        /**
         * эффекты при нажатии на экран
         */
        auto mouse = sf::Mouse::getPosition(window);

        /**
         * проверка на нажатие
         */
        bool contains = mouse.x >= 0 && mouse.x <= edge_size * 32 && mouse.y >= _InterfaceOffset &&
                        mouse.y <= edge_size * 32 + _InterfaceOffset;

        /**
         * если нажали, то проверяем, что там было
         */
        if (contains) {

            /**
             * эта точка, в которую попали мышкой
             */
            auto point = sf::Vector2u(mouse.x / 32, (mouse.y - _InterfaceOffset) / 32);

            /**
             * если левая кнопка мыши нажата
             */
            if (alone::input::isClickedLeftButton()) {

                /**
                 * карта генерируется в момент первого нажатия на карту
                 */
                if (_Revealed == 0) {

                    /**
                     * генерация
                     */
                    _GameMap->generate(_Level, point);

                    /**
                     * устанавливаем текст с количеством оставшихся для поиска бомб
                     */
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(_GameMap->_Bombs));
                    _Revealed++;

                    /**
                     * если сгенерировалось в этой точке ничего, то ищем ближайшие пустые тайлы и с цифрами
                     */
                    if (map[point.x][point.y].second == Type::None)
                        _GameMap->_OpenTiles(point.x, point.y);
                        /**
                         * устанавливаем статус отрисовки "видимый"
                         */
                    else
                        map[point.x][point.y].first = 'r';
                } else {
                    /**
                     * если статус отрисовки "неизвестный"
                     */
                    if (map[point.x][point.y].first == 'n') {
                        if (map[point.x][point.y].second == Type::None)

                            /**
                             * то открываем рядом стоящие пустые тайлы до цифр
                             */
                            _GameMap->_OpenTiles(point.x, point.y);

                            /**
                             * установка статуса "видимый"
                             */
                        else
                            map[point.x][point.y].first = 'r';

                        /**
                         * если игрок нажал по бомбе левой кнопкой мыши, то он проиграл
                         */
                        if (map[point.x][point.y].second == Type::Bomb)
                            /**
                             * Игра окончена
                             */
                            _GameStatus = 'l';

                        _Revealed++;
                    }
                }

                /**
                 * это проверка на нажатие левой кнопкой мыши
                 */
            } else if (alone::input::isClickedRightButton()) {

                /**
                 * если изначально статус тайла был флаг
                 */
                if (map[point.x][point.y].first == 'f') {

                    /**
                     * то меняем его на противоположный
                     */
                    map[point.x][point.y].first = 'n';

                    /**
                     * не забываем обновить счётчик бомб
                     */
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(++_GameMap->_Bombs));

                    /**
                     * и уменьшая количество правильно расположенных на карте флагов
                     */

                    if (map[point.x][point.y].second == Type::Bomb)
                        _Flags--;

                    /**
                     * если же на тайле не было флага
                     */
                } else if (map[point.x][point.y].first == 'n') {

                    /**
                     * то ставим его
                     */
                    map[point.x][point.y].first = 'f';
                    _RemainedLabel.setString("Bombs remained: " + std::to_string(--_GameMap->_Bombs));

                    /**
                     * если всё верно, то инкрементируем счётчик правильных флагов, который не виден игроку
                     */
                    if (map[point.x][point.y].second == Type::Bomb)
                        _Flags++;

                    /**
                     * если все бомбы найдены, то игра заканчивается, а игрок выигрывает
                     */
                    if (_Flags == difficulties[_Level].bombs) {
                        _GameStatus = 'w';
                    }
                }
            }
        }

        /**
         * расчёт вершин для карты
         */
        for (size_t i = 0; i != map.size(); i++) {
            for (size_t j = 0; j != map[i].size(); j++) {
                /**
                 * это 4 вершины одного квадрата
                 */
                auto &top_lhs = _RenderRegion[(i + j * map.size()) * 4];
                auto &top_rhs = _RenderRegion[(i + j * map.size()) * 4 + 1];
                auto &bot_rhs = _RenderRegion[(i + j * map.size()) * 4 + 2];
                auto &bot_lhs = _RenderRegion[(i + j * map.size()) * 4 + 3];

                /**
                 * это id для отрисовки квадрата, показывает, какую точку у атласа с текстурами рисовать
                 */
                size_t id = 0;

                /**
                 * если тайл виден игроку, то
                 */
                if (DEBUG_MODE || map[i][j].first == 'r')
                    /**
                     * просто ставим то, что там есть
                     */
                    id = (size_t) map[i][j].second;
                    /**
                     * если тут флаш, то ставим флаг
                     */
                else if (map[i][j].first == 'f')
                    id = (size_t) Type::Flag;
                else
                    /**
                     * иначе просто рисуем пустоту
                     */
                    id = (size_t) Type::Unknown;


                /**
                 * это id с самой текстурами, так как текстура имеет квадратную форму
                 */
                size_t idx = id % 4;
                size_t idy = id / 4;

                /**
                 * расчет местоположения на экране, включая смещение интерфейса по вертикали
                 */
                top_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32);
                top_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32);
                bot_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32 + 32);
                bot_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32 + 32);

                /**
                 * расчёт 4 вершин с текстуры, которые соответствуют реальной картинке с экрана
                 */
                top_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f);
                top_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f);
                bot_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f + 32);
                bot_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f + 32);
            }
        }

        /**
         * проверка того, закончилась ли игра
         */
        if (_GameStatus != 'a') {
            states.erase("game");
            states.insert("over", std::shared_ptr<State>(new GameOverState(_GameStatus == 'w', _Flags)));
        }
    }

    void onCreate() override {
        /**
         * обнуляем таймер, тк игра началась!
         */
        _Clock.restart();

        /**
         * сбрасываем карту игру и изменяем её размер в зависимости от уровня сложности
         */
        _GameMap.reset(new Map());
        _GameMap->resize(_Level);

        /**
         * количество открытых клеток с карты равно 0
         */
        _Revealed = 0;

        /**
         * размер ребра карты
         */
        size_t edge_size = _GameMap->_Content.size();

        /**
         * размер экрана игры зависит от размера самой карты
         */
        window.setSize(sf::Vector2u(edge_size * 32, edge_size * 32 + _InterfaceOffset));

        /**
         * атлас текстур
         */
        _Atlas = &textures["minesweeper.png"];

        /**
         * установка шрифта для надписей
         */
        _RemainedLabel.setFont(font);
        _TimerLabel.setFont(font);

        /**
         * цвет внутри
         */
        _RemainedLabel.setFillColor(sf::Color::White);
        _TimerLabel.setFillColor(sf::Color::White);

        /**
         * снаружи
         */
        _RemainedLabel.setOutlineColor(sf::Color::White);
        _TimerLabel.setOutlineColor(sf::Color::White);

        /**
         * координаты надписей
         */
        _RemainedLabel.setPosition(20, 15);
        _TimerLabel.setPosition(20, 50);

        /**
         *  установка специального размера текста для самого лёгкого уровня сложности
         */
        if (_Level == 0) {
            _RemainedLabel.setCharacterSize(24);
            _TimerLabel.setCharacterSize(24);
        }
    }

    /**
     * тут же при удалении лучше перестраховаться и обнулить умный указатель
     */
    void onDelete() override {
        _GameMap.reset(nullptr);
    }

    /**
     * отрисовка самой игры
     * @param target
     * @param states
     */
    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override {
        states.texture = _Atlas;
        target.draw(_RenderRegion, states);

        target.draw(_RemainedLabel, states);
        target.draw(_TimerLabel, states);
    }
};

MenuState::MenuState() {
    /**
     * Дополняем поведение кнопок в случае нажатия для меню
     */
    _Params = {
            std::make_pair(std::string("Easy"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(0)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Medium"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(1)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Hard"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(2)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Exit"), []() {
                window.close();
            })
    };
}

void init() {
    textures.load("assets/textures/include.txt");

    /**
     * заполнением параметров уровня сложности
     */
    difficulties[0] = {"Easy", 10, 8};
    difficulties[1] = {"Medium", 20, 10};
    difficulties[2] = {"Hard", 70, 20};

    /**
     * погружаем шрифт
     */
    font.loadFromFile("assets/font.ttf");

    /**
     * добавляем меню как активное состояние игры
     */
    states.insert("menu", std::shared_ptr<alone::State>(new MenuState()));
}

int main() {
    init();

    sf::Music music;


    music.openFromFile("mysaca.ogg");
    music.setVolume(50);

    music.play();
    while (window.isOpen()) {

        /**
         * это проверка ивентов самого окна
         */
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {

                /**
                 * если окно было закрыто
                 */
                case sf::Event::Closed:
                    window.close();
                    break;

                    /**
                     * /если окну поменяли размер
                     */
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    break;
            }
        }

        /**
         * очищаем окно
         */
        window.clear();

        /**
         * обновляем ввод
         */
        alone::input::update();

        /**
         * а затем все состояния
         */
        states.update();

        /**
         * выводим на экран
         */
        window.display();
    }

    return 0;
}


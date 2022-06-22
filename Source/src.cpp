#include "src.h"

void alone::TextureManager::load(std::string config_name) {
    std::ifstream file(config_name);

    while (!file.eof()) {
        std::string temp;
        file >> temp;

        sf::Texture texture;
        texture.loadFromFile("assets/textures/" + temp);

        _Content.emplace(temp, std::move(texture));
    }
}

sf::Texture& alone::TextureManager::operator[](std::string key) {
    return _Content.at(key);
}

void alone::StateMachine::insert(std::string key, std::shared_ptr <State> value) {
    value->_Status = State::OnCreate;
    _Content.emplace(key, value);
}

void alone::StateMachine::erase(std::string key) {
    auto it = _Content.find(key);
    if (it != _Content.end())
        it->second->_Status = State::OnDelete;
}

void alone::StateMachine::update() {
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

void alone::input::update() {
    preLmb = nowLmb;
    preRmb = nowRmb;
    nowLmb = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    nowRmb = sf::Mouse::isButtonPressed(sf::Mouse::Right);
}

bool alone::input::isClickedLeftButton() {
    return preLmb && !nowLmb;
}

bool alone::input::isClickedRightButton() {
    return preRmb && !nowRmb;
}

void Map::resize(size_t level) {
    auto& d = difficulties[level];

    _Content.resize(d.size);
    for (auto& it : _Content)
        it.resize(d.size, { 'n', Type::None});
}

void Map::generate(size_t level, sf::Vector2u point) {
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

bool Map::_HasBomb(size_t x, size_t y) {
    if (x < 0 || x >= _Content.size() || y < 0 || y >= _Content.size())
        return false;
    return _Content[x][y].second == Type::Bomb;
};

size_t Map::_DetectAround(size_t x, size_t y) {
    return _HasBomb(x - 1, y - 1) + _HasBomb(x, y - 1) + _HasBomb(x + 1, y - 1) +
           _HasBomb(x - 1, y) + _HasBomb(x + 1, y) +
           _HasBomb(x - 1, y + 1) + _HasBomb(x, y + 1) + _HasBomb(x + 1, y + 1);
};

void Map::_OpenTiles(int x, int y) {
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

void MenuState::update(){
    auto mouse = sf::Mouse::getPosition(window);

    for (size_t i = 0; i != _Buttons.size(); i++) {
        auto bounds = _Buttons[i].getGlobalBounds();
        //std::cout << mouse.x << ' ' << mouse.y << ' ' << alone::input::isClicked() << '\n';
        if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
            _Params[i].second();
        }
    }
}

void MenuState::onCreate(){
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

void MenuState::onDelete(){

}

void MenuState::draw(sf::RenderTarget& target, sf::RenderStates states) const{
    for (const auto& it : _Buttons)
        target.draw(it, states);
}

void GameOverState::update(){
    auto mouse = sf::Mouse::getPosition(window);
    auto bounds = _Exit.getGlobalBounds();

    if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
        states.erase("over");
        states.insert("menu", std::shared_ptr <State>(new MenuState()));
    }
}

void GameOverState::onCreate(){
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

void GameOverState::onDelete()
{

}

void GameOverState::draw(sf::RenderTarget& target, sf::RenderStates states) const{
    target.draw(_Label, states);
    target.draw(_Exit, states);
}

void GameState::update(){
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

void GameState::onCreate(){
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

void GameState::onDelete(){
    _GameMap.reset(nullptr);
}

void GameState::draw(sf::RenderTarget& target, sf::RenderStates states) const{
    states.texture = _Atlas;
    target.draw(_RenderRegion, states);

    target.draw(_RemainedLabel, states);
    target.draw(_TimerLabel, states);
}

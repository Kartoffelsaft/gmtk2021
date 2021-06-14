#include <SFML/Graphics.hpp>
#include <unordered_set>
#include <chrono>
#include <cmath>
#include <random>

struct Chain {
    sf::Vector2f endPoint;
    sf::Vector2f velocity;
    sf::Vector2f const * root;
    sf::Vector2f * rootVel;
    float length;
};

struct Enemy {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float size;
};

float getDistSqrRatio(sf::Vector2f const from, sf::Vector2f const to, float length) {
    auto dx = (from.x - to.x);
    auto dy = (from.y - to.y);

    return (dx * dx + dy * dy) / (length * length);
}

int main(int argc, char const argv[]) {
    auto windowWidth = 1600;
    auto windowHeight = 900;
    sf::RenderWindow window{sf::VideoMode(windowWidth, windowHeight), "Game", sf::Style::Close | sf::Style::Titlebar};

    auto rng = std::mt19937{std::random_device{}()};

    std::unordered_set<sf::Keyboard::Key> keysHeld;
    std::unordered_set<sf::Keyboard::Key> keysJustPressed;


    unsigned int score;
    unsigned int scoreMultiplier;
    bool lost;


    sf::Vector2f playerPos;
    sf::Vector2f playerVel;
    auto const playerAccel = 5000.f;
    auto const playerVelLoss = 8.f;
    auto playerShape = sf::RectangleShape{sf::Vector2f{60, 60}};
    playerShape.setFillColor(sf::Color{100, 120, 220});
    int8_t playerHealth;


    std::vector<Chain> chainLinks;
    int const initialchainLinkCount = 10;
    chainLinks.reserve(initialchainLinkCount);
    auto const chainVelLoss = 4.f;
    sf::Vector2f chainGrav;


    sf::Vector2f whackPos;
    auto const whackSize = 25.f;


    std::vector<Enemy> enemies;
    int totalEnemies;


    std::vector<sf::Vector2f> multiplierPickups;
    float totalMultiplierPickups;
    auto const maxMultiplierCooldown = 10.f;
    float currentMultiplierCooldown;


    std::chrono::high_resolution_clock::time_point frameStart;
    float frameTime;

    std::chrono::high_resolution_clock::time_point beginTime;

    auto setup = [&](){
        score = 0;
        scoreMultiplier = 1;
        lost = false;


        playerPos = sf::Vector2f(windowWidth / 2, windowHeight / 2);
        playerVel = sf::Vector2f(0, 0);
        playerHealth = 3;


        chainLinks.clear();
        int const initialchainLinkCount = 10;
        for(int i = 0; i < initialchainLinkCount; i++) {
            sf::Vector2f const * nRoot;
            sf::Vector2f * nRootVel;
            if(i == 0) {
                nRoot = &playerPos;
                nRootVel = &playerVel;
            } else {
                nRoot = &chainLinks[i - 1].endPoint;
                nRootVel = &chainLinks[i - 1].velocity;
            }

            chainLinks.push_back(Chain{
                .endPoint = sf::Vector2f{playerPos.x, playerPos.y + 60 + 30 * i},
                .velocity = sf::Vector2f{0, 0},
                .root = nRoot,
                .rootVel = nRootVel,
                .length = 30
            });
        }
        chainLinks[0].length = 60;
        chainGrav = sf::Vector2f(0, 0);


        whackPos = chainLinks.back().endPoint;


        enemies.clear();
        totalEnemies = 0;


        multiplierPickups.clear();
        totalMultiplierPickups = 0;
        currentMultiplierCooldown = 0.f;


        frameStart = std::chrono::high_resolution_clock::now();
        frameTime = 0.f;

        beginTime = std::chrono::high_resolution_clock::now();
    };
    setup();

    auto uniformDistribution = [](auto& rand){
        uint64_t n = rand();
        
        return (float)(n - rand.min()) / (float)(rand.max() - rand.min());
    };

    auto spawnEnemy = [&](){
        auto spawnDistributorRough = std::discrete_distribution<>{{1, 0, 0, 1}};

        auto nPos = sf::Vector2f{
            (float)(spawnDistributorRough(rng) - 2) * 1600 + uniformDistribution(rng) * 900 + windowWidth / 2,
            (float)(spawnDistributorRough(rng) - 2) * 900 + uniformDistribution(rng) * 600 + windowHeight / 2,
        };

        auto target = sf::Vector2f{
            (uniformDistribution(rng) - 0.5f) * 500 + windowWidth / 2,
            (uniformDistribution(rng) - 0.5f) * 500 + windowHeight / 2,
        };

        enemies.push_back(Enemy{
            .position = nPos,
            .velocity = (nPos - target) * -0.10f * uniformDistribution(rng),
            .size = 10.f
        });

        totalEnemies++;
    };

    auto drawNumber = [&](uint8_t what, sf::Vector2f at, sf::Vector2f scale = sf::Vector2f{12, 24}) {
        switch (what) {
            case 0: {
                auto lines = sf::VertexArray{sf::LinesStrip, 6};
                lines[0].position = sf::Vector2f{1 * scale.x, 0 * scale.y} + at;
                lines[1].position = sf::Vector2f{1 * scale.x, 1 * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 1 * scale.y} + at;
                lines[3].position = sf::Vector2f{0 * scale.x, 0 * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 0 * scale.y} + at;
                lines[5].position = sf::Vector2f{0 * scale.x, 1 * scale.y} + at;
                window.draw(lines);
                break;
            } case 1: {
                auto lines = sf::VertexArray{sf::LinesStrip, 5};
                lines[0].position = sf::Vector2f{0.25 * scale.x, 0.25 * scale.y} + at;
                lines[1].position = sf::Vector2f{0.5  * scale.x, 0    * scale.y} + at;
                lines[2].position = sf::Vector2f{0.5  * scale.x, 1    * scale.y} + at;
                lines[3].position = sf::Vector2f{0    * scale.x, 1    * scale.y} + at;
                lines[4].position = sf::Vector2f{1    * scale.x, 1    * scale.y} + at;
                window.draw(lines);
                break;
            } case 2: {
                auto lines = sf::VertexArray{sf::LinesStrip, 6};
                lines[0].position = sf::Vector2f{0 * scale.x, 0    * scale.y} + at;
                lines[1].position = sf::Vector2f{1 * scale.x, 0    * scale.y} + at;
                lines[2].position = sf::Vector2f{1 * scale.x, 0.25 * scale.y} + at;
                lines[3].position = sf::Vector2f{0 * scale.x, 0.75 * scale.y} + at;
                lines[4].position = sf::Vector2f{0 * scale.x, 1    * scale.y} + at;
                lines[5].position = sf::Vector2f{1 * scale.x, 1    * scale.y} + at;
                window.draw(lines);
                break;
            } case 3: {
                auto lines = sf::VertexArray{sf::LinesStrip, 7};
                lines[0].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[1].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[2].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[3].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[5].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                lines[6].position = sf::Vector2f{0 * scale.x, 1   * scale.y} + at;
                window.draw(lines);
                break;
            } case 4: {
                auto lines = sf::VertexArray{sf::LinesStrip, 5};
                lines[0].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[1].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                lines[2].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[3].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                window.draw(lines);
                break;
            } case 5: {
                auto lines = sf::VertexArray{sf::LinesStrip, 6};
                lines[0].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[1].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                lines[3].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                lines[5].position = sf::Vector2f{0 * scale.x, 1   * scale.y} + at;
                window.draw(lines);
                break;
            } case 6: {
                auto lines = sf::VertexArray{sf::LinesStrip, 6};
                lines[0].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[1].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 1   * scale.y} + at;
                lines[3].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[5].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                window.draw(lines);
                break;
            } case 7: {
                auto lines = sf::VertexArray{sf::LinesStrip, 3};
                lines[0].position = sf::Vector2f{0 * scale.x, 0 * scale.y} + at;
                lines[1].position = sf::Vector2f{1 * scale.x, 0 * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 1 * scale.y} + at;
                window.draw(lines);
                break;
            } case 8: {
                auto lines = sf::VertexArray{sf::LinesStrip, 7};
                lines[0].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[1].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[3].position = sf::Vector2f{0 * scale.x, 1   * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                lines[5].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[6].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                window.draw(lines);
                break;
            } case 9: {
                auto lines = sf::VertexArray{sf::LinesStrip, 6};
                lines[0].position = sf::Vector2f{1 * scale.x, 0.5 * scale.y} + at;
                lines[1].position = sf::Vector2f{0 * scale.x, 0.5 * scale.y} + at;
                lines[2].position = sf::Vector2f{0 * scale.x, 0   * scale.y} + at;
                lines[3].position = sf::Vector2f{1 * scale.x, 0   * scale.y} + at;
                lines[4].position = sf::Vector2f{1 * scale.x, 1   * scale.y} + at;
                lines[5].position = sf::Vector2f{0 * scale.x, 1   * scale.y} + at;
                window.draw(lines);
                break;
            } 
        }
    };

    auto drawGui = [&](){
        auto healthBackground = sf::RectangleShape{sf::Vector2f{190, 119}};
        healthBackground.setPosition(sf::Vector2f{20, 20});
        healthBackground.setFillColor(sf::Color::Black);
        healthBackground.setOutlineColor(sf::Color::Blue);
        healthBackground.setOutlineThickness(1.f);
        window.draw(healthBackground);

        for(int8_t i = 0; i < playerHealth; i++) {
            auto healthBar = sf::RectangleShape{sf::Vector2f{50, 30}};
            healthBar.setPosition(sf::Vector2f{i * 60 + 30, 30});
            healthBar.setFillColor(sf::Color::Red);

            window.draw(healthBar);
        }

        float scoreCharOffset = 0.f;
        for(char c: std::to_string(score)) {
            drawNumber(c - 0x30, sf::Vector2f{scoreCharOffset + 30, 70});
            scoreCharOffset += 16;
        }

        auto xShape = sf::VertexArray{sf::Lines, 4};
        xShape[0].position = sf::Vector2f{30, 117};
        xShape[1].position = sf::Vector2f{42, 129};
        xShape[2].position = sf::Vector2f{30, 129};
        xShape[3].position = sf::Vector2f{42, 117};
        window.draw(xShape);

        auto cooldownShape = sf::RectangleShape{sf::Vector2f{5, currentMultiplierCooldown / maxMultiplierCooldown * 24}};
        cooldownShape.setPosition(sf::Vector2f{46, 105});
        window.draw(cooldownShape);

        scoreCharOffset = 0.f;
        for(char c: std::to_string(scoreMultiplier)) {
            drawNumber(c - 0x30, sf::Vector2f{scoreCharOffset + 55, 105});
            scoreCharOffset += 16;
        }
    };

    while (window.isOpen())
    {
        sf::Event event;
        keysJustPressed.clear();
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            
            else if (event.type == sf::Event::KeyPressed) {
                keysHeld.insert(event.key.code);
                keysJustPressed.insert(event.key.code);
            }

            else if (event.type == sf::Event::KeyReleased) {
                keysHeld.erase(event.key.code);
            }
        }

        if(keysJustPressed.contains(sf::Keyboard::R)) setup();


        if(!lost) {
            playerVel *= std::exp(-1.f * playerVelLoss * frameTime);

            if(keysHeld.contains(sf::Keyboard::D)) {
                playerVel.x += playerAccel * frameTime;
            }
            if(keysHeld.contains(sf::Keyboard::A)) {
                playerVel.x -= playerAccel * frameTime;
            }
            if(keysHeld.contains(sf::Keyboard::S)) {
                playerVel.y += playerAccel * frameTime;
            }
            if(keysHeld.contains(sf::Keyboard::W)) {
                playerVel.y -= playerAccel * frameTime;
            }

            chainGrav = sf::Vector2f{0, 0};
            if(keysHeld.contains(sf::Keyboard::Right)) {
                chainGrav.x += 20000;
            }
            if(keysHeld.contains(sf::Keyboard::Left)) {
                chainGrav.x -= 20000;
            }
            if(keysHeld.contains(sf::Keyboard::Down)) {
                chainGrav.y += 20000;
            }
            if(keysHeld.contains(sf::Keyboard::Up)) {
                chainGrav.y -= 20000;
            }

            playerPos += playerVel * frameTime;
            playerShape.setPosition(playerPos - sf::Vector2f{30, 30});


            for(auto& link: chainLinks) {
                link.velocity += frameTime * chainGrav;
                link.endPoint += frameTime * link.velocity;

                auto overlength = getDistSqrRatio(link.endPoint, *link.root, link.length);
                if(overlength > 1.05f) {
                    auto actualOverlength = std::sqrt(overlength);
                    auto dir = *link.root - link.endPoint;
                    link.endPoint += dir * (1.f - 1.f / actualOverlength);

                    auto dirNorm = dir / (actualOverlength * link.length);
                    auto dirRot = sf::Vector2f(dirNorm.y, -dirNorm.x);
                    link.velocity = dirRot * (link.velocity.x * dirRot.x + link.velocity.y * dirRot.y);
                    link.velocity *= std::exp(-1.f * chainVelLoss * frameTime);
                }
            }


            whackPos = chainLinks.back().endPoint;
            auto whackShape = sf::CircleShape{whackSize};
            whackShape.setPosition(whackPos - sf::Vector2f{1, 1} * whackSize);
            whackShape.setFillColor(sf::Color{100, 100, 100});


            if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - beginTime).count() > std::sqrt(totalEnemies) * 5) spawnEnemy();
            for(auto& enemy: enemies) {
                enemy.position += frameTime * enemy.velocity;
            }

            for(auto i = enemies.begin(); i < enemies.end(); i++) {
                if(getDistSqrRatio(i->position, whackPos, i->size + whackSize) < 1.f) {
                    enemies.erase(i);
                    score += 100 * scoreMultiplier;
                    i--;
                }
                if(i->position.x < playerPos.x + 30
                && i->position.x > playerPos.x - 30
                && i->position.y < playerPos.y + 30
                && i->position.y > playerPos.y - 30
                ) {
                    enemies.erase(i);
                    playerHealth--;
                    if(playerHealth <= 0) lost = true;
                    i--;
                }
                if(i->position.x < -5000
                || i->position.x > windowWidth + 5000
                || i->position.y < -5000
                || i->position.y > windowHeight + 5000
                ) {
                    enemies.erase(i);
                    i--;
                }
            }


            if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - beginTime).count() > totalMultiplierPickups * 3) {
                multiplierPickups.push_back(sf::Vector2f{
                    uniformDistribution(rng) * windowWidth,
                    uniformDistribution(rng) * windowHeight
                });
                totalMultiplierPickups++;
            }

            if(scoreMultiplier > 1) {
                currentMultiplierCooldown -= frameTime;
                if(currentMultiplierCooldown < 0) {
                    currentMultiplierCooldown = 0;
                    scoreMultiplier = 1;
                }
            }

            for(auto i = multiplierPickups.begin(); i < multiplierPickups.end(); i++) {
                if(i->x < playerPos.x + 30
                && i->x > playerPos.x - 30
                && i->y < playerPos.y + 30
                && i->y > playerPos.y - 30
                ) {
                    multiplierPickups.erase(i);
                    scoreMultiplier++;
                    currentMultiplierCooldown = maxMultiplierCooldown;
                    i--;
                }
            }


            auto chain = sf::VertexArray{sf::LinesStrip, chainLinks.size() + 1};
            chain[0] = playerPos;
            for(int i = 0; i < chainLinks.size(); i++) {
                chain[i + 1].position = chainLinks[i].endPoint;
            }

            window.draw(chain);
            window.draw(playerShape);

            for(auto const & pickup: multiplierPickups) {
                auto shape = sf::CircleShape{5.f};
                shape.setFillColor(sf::Color::Blue);
                shape.setPosition(pickup - sf::Vector2f{2.5f, 2.5f});

                window.draw(shape);
            }

            window.draw(whackShape);
        }

        for(auto const & enemy: enemies){
            auto shape = sf::CircleShape{enemy.size};
            shape.setFillColor(sf::Color::Red);
            shape.setPosition(enemy.position - sf::Vector2f{1, 1} * enemy.size);

            window.draw(shape);
        }

        drawGui();

        window.display();
        window.clear(
            lost? sf::Color{30, 0, 0}
                : sf::Color::Black
        );

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> frameChrono = frameEnd - frameStart;
        frameTime = frameChrono.count();
        frameStart = frameEnd;
    }

    return 0;
}
#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#pragma region("GAME CONSTANTS")
constexpr int STRIKE_RADIUS = 3;
constexpr int MAX_MAP_X = 40;
constexpr int MIN_MAP_X = 0;
constexpr int MAX_MAP_Y = 18;
constexpr int MIN_MAP_Y = 0;
#pragma endregion

#pragma region("INPUT UTILS")
template <typename T>
T Read(std::istream& input) {
    T result;
    input >> result;
    return result;
}
#pragma endregion

#pragma region("MATH UTILS")
constexpr int INF = std::numeric_limits<int>::max();

struct Point {
    int X = 0;
    int Y = 0;
    
    static Point FromStream(std::istream& input) {
        const auto x = Read<int>(input);
        const auto y = Read<int>(input);
        return {x, y};
    }
};

Point operator+(const Point& lhs, const Point& rhs) {
    return {lhs.X + rhs.X, lhs.Y + rhs.Y};
}

Point operator-(const Point& lhs, const Point& rhs) {
    return {lhs.X - rhs.X, lhs.Y - rhs.Y};
}

int Distance(const Point& lhs, const Point& rhs) {
    const int dxAbs = abs(lhs.X - rhs.X);
    const int dyAbs = abs(lhs.Y - rhs.Y);
    return std::max(dxAbs, dyAbs);
}

bool IsInRadius(const Point& point, const Point& center, int radius) {
    return Distance(point, center) <= radius;
}

std::ostream& WritePoint(std::ostream& os, const Point& p, const std::string& msg = "") {
    if (!msg.empty()) {
        os << msg << ": ";
    }
    os << p.X << "; " << p.Y;
    return os;
}
#pragma endregion

#pragma region("GAME-SPECIFIC UTILS")
bool IsOnMap(const Point& p) {
    return p.X >= MIN_MAP_X && p.X < MAX_MAP_X
           && p.Y >= MIN_MAP_Y && p.Y < MAX_MAP_Y;
}

const std::vector<Point>& GetPossibleDirections() {
    static const std::vector<Point> directions = {
        {-1, -1},
        {-1, 0},
        {-1, 1},
        {0, -1},
        {0, 0},
        {0, 1},
        {1, -1},
        {1, 0},
        {1, 1}
    };
    return directions;
}

std::string GetSymbolicDirection(const Point& current, const Point& desired) {
    std::string result;
    const auto dir = desired - current;
    
    if (dir.Y > 0) {
        result += "S";
    } else if (dir.Y < 0) {
        result += "N";
    }
    
    if (dir.X > 0) {
        result += "E";
    } else if (dir.X < 0) {
        result += "W";
    }
    
    if (result.empty()) {
        result = "WAIT";
    }

    return result;
}
#pragma endregion

#pragma region("GAME ENTITIES")
class Thor {
public:
    Thor(const Point& position, int strikesLeft)
        : Position(position)
        , StrikesLeft(strikesLeft)
    {
    }
    
    void SetPosition(const Point& position) {
        Position = position;
    }
    
    void Strike() {
        if (StrikesLeft <= 0) {
            throw "Not enough charges!";
        }
        --StrikesLeft;
    }
    
    const Point GetPosition() const {
        return Position;
    }
    
    int GetStrikes() const {
        return StrikesLeft;
    }

private:
    Point Position;
    int StrikesLeft = 0;
};

class Monster {
public:
    using ListType = std::vector<Monster>;

public:
    explicit Monster(const Point& position)
        : Position(position)
    {
    }
    
    const Point& GetPosition() const {
        return Position;
    }

private:
    Point Position;
};
#pragma endregion

#pragma region("STRATEGY")
class IStrategy {
public:
    virtual ~IStrategy() = default;
    
    virtual std::string MakeDecision(const Monster::ListType& monsters, Thor& thor) = 0;
};

class SimpleStrategy final : public IStrategy {
public:
    std::string MakeDecision(const Monster::ListType& monsters, Thor& thor) override {
        if (monsters.size() == 1) {
            const auto& monster = monsters.back();        
            if (IsInRadius(monster.GetPosition(), thor.GetPosition(), STRIKE_RADIUS)) {
                return "STRIKE";
            }
        }
        
        return "WAIT";
    }
};

class FollowMostDistant : public IStrategy {
public:
    std::string MakeDecision(const Monster::ListType& monsters, Thor& thor) override {
        if (monsters.empty()) {
            return "WAIT";
        }
        
        auto allowedPositions = GetAllowedPositions(monsters, thor);
        std::cerr << "Allowed positions: " << allowedPositions.size() << std::endl;
        if (allowedPositions.empty()) {
            thor.Strike();
            return "STRIKE";
        }
        
        const auto& mostDistantMonster = FindMostDistantMonster(monsters, thor);
        if (Distance(mostDistantMonster.GetPosition(), thor.GetPosition()) <= STRIKE_RADIUS) {
            thor.Strike();
            return "STRIKE";
        }
        
        const auto nextPosition = GetNextPosition(mostDistantMonster, allowedPositions);
        WritePoint(std::cerr, thor.GetPosition(), "Thor") << std::endl;
        WritePoint(std::cerr, mostDistantMonster.GetPosition(), "Monster") << std::endl;
        WritePoint(std::cerr, nextPosition, "Next position") << std::endl;
        
        return MoveThor(nextPosition, thor);
    }
    
private:
    std::string MoveThor(const Point& nextPosition, Thor& thor) {
        const auto dir = GetSymbolicDirection(thor.GetPosition(), nextPosition);
        thor.SetPosition(nextPosition);
        return dir;
    }

    Point GetNextPosition(const Monster& mostDistantMonster, std::vector<Point>& allowedPositions) {
        const auto& monsterPosition = mostDistantMonster.GetPosition();
        std::sort(allowedPositions.begin(), allowedPositions.end(), [&](const Point& lhs, const Point& rhs) {
            const auto lhsDistance = Distance(lhs, monsterPosition);
            const auto rhsDistance = Distance(rhs, monsterPosition);
            return lhsDistance < rhsDistance;
        });
        
        for (const auto& pos : allowedPositions) {
            WritePoint(std::cerr, pos, "Point") << ", cost: " << Distance(pos, monsterPosition) << std::endl;
        }
        
        return allowedPositions.front();
    }

    static std::vector<Point> GetAllowedPositions(const Monster::ListType& monsters, const Thor& thor) {
        std::vector<Point> result;
        const auto playerPosition = thor.GetPosition();
        
        for (const auto& dir : GetPossibleDirections()) {
            const auto nextPosition = playerPosition + dir;
            if (!IsOnMap(nextPosition)) {
                continue;
            }
            
            auto minDistance = INF;
            for (const auto& monster : monsters) {
                const auto monsterPosition = monster.GetPosition();
                const auto distance = Distance(nextPosition, monsterPosition);
                if (distance < minDistance) {
                    minDistance = distance;
                }
            }
            
            if (minDistance > 1) {
                result.push_back(nextPosition);
            }
        }
        
        return result;
    }

    static const Monster& FindMostDistantMonster(const Monster::ListType& monsters, const Thor& thor) {
        size_t maxIdx = 0;
        int maxDistance = 0;
        for (size_t i = 0; i < monsters.size(); ++i) {
            const auto& monster = monsters[i];
            const int distanceToMonster = Distance(monster.GetPosition(), thor.GetPosition());
            if (maxDistance < distanceToMonster) {
                maxDistance = distanceToMonster;
                maxIdx = i;
            }
        }
        return monsters[maxIdx];
    }
};

std::unique_ptr<IStrategy> CreateSimpleStrategy() {
    return std::make_unique<SimpleStrategy>();
}

std::unique_ptr<IStrategy> CreateMainStrategy() {
    return std::make_unique<FollowMostDistant>();
}
#pragma endregion

class World {
public:
    explicit World(std::istream& input)
        : Player(ReadPlayer(input))
        , Monsters(ReadMonsters(input))
        , Strategy(CreateMainStrategy())
    {
    }

    void NextStep(std::istream& input, std::ostream& output) {
        output << Strategy->MakeDecision(Monsters, Player) << std::endl;
        
        Read<int>(input); // skip the remaining number of hammer strikes
        Monsters = ReadMonsters(input);
        
        WriteWorldMap(std::cerr);
    }

    bool IsRunning() const {
        return true;
    }

private:
    Thor Player;
    Monster::ListType Monsters;
    std::unique_ptr<IStrategy> Strategy;

private:
    static Thor ReadPlayer(std::istream& input) {
        const auto position = Point::FromStream(input);
        const auto strikesLeft = Read<int>(input);
        return {position, strikesLeft};
    }

    static Monster::ListType ReadMonsters(std::istream& input) {
        const int amount = Read<int>(input);
        Monster::ListType result;
        for (int i = 0; i < amount; ++i) {
            result.emplace_back(Point::FromStream(input));
        }
        return result;
    }
    
    void WriteWorldMap(std::ostream& os) const {
        static const auto toPlainIdx = [this](const Point& p) {
            return MAX_MAP_X * p.Y + p.X;
        };
        
        std::vector<char> worldMap(MAX_MAP_X * MAX_MAP_Y);
        std::fill(worldMap.begin(), worldMap.end(), '.');
        for (const auto& monster : Monsters) {
            worldMap[toPlainIdx(monster.GetPosition())] = 'G';
        }
        worldMap[toPlainIdx(Player.GetPosition())] = 'T';
        
        os << "*" << std::string(2 * MAX_MAP_X - 1, '-') << "*\n|";
        for (size_t i = 0; i < worldMap.size(); ++i) {
            if (i != 0 && i % MAX_MAP_X == 0) {
                os << "\n|";
            }
            os << worldMap[i] << "|";
        }
        os << "\n*" << std::string(2 * MAX_MAP_X - 1, '-') << "*" << std::endl;
    }
};

int main(int argc, const char** argv) {
    World world(std::cin);

    while (world.IsRunning()) {
        world.NextStep(std::cin, std::cout);
    }

    return 0;
}

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#define PRINT_DEBUG_INFO

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

int DotProduct(const Point& lhs, const Point& rhs) {
    return lhs.X * rhs.X + lhs.Y * rhs.Y;
}

int ManhattanDistance(const Point& lhs, const Point& rhs) {
    const int dxAbs = abs(lhs.X - rhs.X);
    const int dyAbs = abs(lhs.Y - rhs.Y);
    return std::max(dxAbs, dyAbs);
}

float EuclidDistance(const Point& lhs, const Point& rhs) {
    const auto diff = rhs - lhs;
    return sqrtf(static_cast<float>(DotProduct(diff, diff)));
}

bool IsInRadius(const Point& point, const Point& center, int radius) {
    return ManhattanDistance(point, center) <= radius;
}

std::ostream& WritePoint(std::ostream& os, const Point& p, const std::string& msg = "") {
    if (!msg.empty()) {
        os << msg << ": ";
    }
    os << p.X << "; " << p.Y;
    return os;
}

template <typename T>
class Matrix {
public:
    Matrix(size_t columns, size_t rows)
        : Columns(columns)
        , Rows(rows)
        , Data(Columns * Rows)
    {
    }

    Matrix(size_t columns, size_t rows, const T& defaultValue)
        : Columns(columns)
        , Rows(rows)
        , Data(Columns * Rows, defaultValue)
    {
    }

    const T& Get(size_t column, size_t row) const {
        return Data[AsRawIndex(column, row)];
    }

    void Set(size_t column, size_t row, const T& value) {
        Data[AsRawIndex(column, row)] = value;
    }

    void Clear(const T& value) {
        std::fill(Data.begin(), Data.end(), value);
    }

private:
    using DataHolderType = std::vector<T>;

private:
    size_t Columns;
    size_t Rows;
    DataHolderType Data;

private:
    size_t AsRawIndex(size_t column, size_t row) const {
        return static_cast<size_t>(row * Columns + column);
    }
};
#pragma endregion

#pragma region("GAME-SPECIFIC UTILS")
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
    Thor(const Point& position, int strikeRadius, int strikesLeft)
        : Position(position)
        , StrikeRadius(strikeRadius)
        , StrikesLeft(strikesLeft)
    {
    }

    const Point GetPosition() const {
        return Position;
    }

    int GetStrikes() const {
        return StrikesLeft;
    }

    bool CanStrike(const Point& position) const {
        return ManhattanDistance(position, Position) <= StrikeRadius;
    }

    void SetPosition(const Point& position) {
        Position = position;
    }

    void Strike() {
        if (StrikesLeft <= 0) {
            throw std::runtime_error("Not enough charges!");
        }
        --StrikesLeft;
    }

private:
    Point Position;
    const int StrikeRadius;
    int StrikesLeft = 0;
};

class Giant {
public:
    using ListType = std::vector<Giant>;

public:
    explicit Giant(const Point& position)
        : Position(position)
    {
    }

    const Point& GetPosition() const {
        return Position;
    }

private:
    Point Position;
};

class GameWorldMap {
public:
    enum class CellType : uint16_t {
        EMPTY,
        THOR,
        GIANT
    };

public:
    GameWorldMap(int mapWidth, int mapHeight)
        : MapWidth(mapWidth)
        , MapHeight(mapHeight)
        , Cells(static_cast<size_t>(MapWidth), static_cast<size_t>(MapHeight), CellType::EMPTY)
    {
    }

    int GetMapWidth() const {
        return MapWidth;
    }

    int GetMapHeight() const {
        return MapHeight;
    }

    CellType GetEntity(const Point& position) const {
        return Cells.Get(static_cast<size_t>(position.X), static_cast<size_t>(position.Y));
    }

    bool IsOnMap(const Point& position) const {
        return (position.X >= 0 && position.X < MapWidth) && (position.Y >= 0 && position.Y < MapHeight);
    }

    bool HasGiant(const Point& position) const {
        return GetEntity(position) == CellType::GIANT;
    }

    void Clear(const Point& position) {
        Set(position, CellType::EMPTY);
    }

    void Clear() {
        Cells.Clear(CellType::EMPTY);
    }

    void PlaceThor(const Point& position) {
        Set(position, CellType::THOR);
    }

    void PlaceGiant(const Point& position) {
        Set(position, CellType::GIANT);
    }

private:
    const int MapWidth;
    const int MapHeight;
    Matrix<CellType> Cells;

private:
    void Set(const Point& position, CellType value) {
        Cells.Set(static_cast<size_t>(position.X), static_cast<size_t>(position.Y), value);
    }
};
#pragma endregion

#pragma region("STRATEGY")
class IStrategy {
public:
    virtual ~IStrategy() = default;

    virtual std::string MakeDecision() = 0;
};

class FollowMostDistant final : public IStrategy {
public:
    FollowMostDistant(const GameWorldMap& worldMap, const Giant::ListType& giants, Thor& thor)
        : WorldMap(worldMap)
        , Giants(giants)
        , Player(thor)
    {
    }

    std::string MakeDecision() override {
        if (Giants.empty()) {
            return "WAIT";
        }

        auto allowedPositions = FindAllowedPositions();
        if (allowedPositions.empty()) {
            Player.Strike();
            return "STRIKE";
        }

        const auto& mostDistantGiant = FindMostDistantGiant();
        if (Player.CanStrike(mostDistantGiant.GetPosition())) {
            Player.Strike();
            return "STRIKE";
        }

        const auto nextPosition = FindNextPosition(mostDistantGiant, allowedPositions);

        #ifdef PRINT_DEBUG_INFO
        WritePoint(std::cerr, Player.GetPosition(), "Thor") << std::endl;
        WritePoint(std::cerr, mostDistantGiant.GetPosition(), "Giant") << std::endl;
        WritePoint(std::cerr, nextPosition, "Next position") << std::endl;
        #endif

        return MoveThor(nextPosition);
    }

private:
    using DistanceMap = Matrix<int>;

private:
    const GameWorldMap& WorldMap;
    const Giant::ListType& Giants;
    Thor& Player;

private:
    std::string MoveThor(const Point& nextPosition) {
        const auto dir = GetSymbolicDirection(Player.GetPosition(), nextPosition);
        Player.SetPosition(nextPosition);
        return dir;
    }

    Point FindNextPosition(const Giant& mostDistantGiant, std::vector<Point>& allowedPositions) const {
        const auto& playerPosition = Player.GetPosition();
        const auto& giantPosition = mostDistantGiant.GetPosition();
        const auto distances = FindDistancesToPoint(giantPosition);
        if (distances.Get(playerPosition.X, playerPosition.Y) != INF) {
            std::sort(allowedPositions.begin(), allowedPositions.end(), [&](const Point& lhs, const Point& rhs) {
                const auto lhsDistance = distances.Get(lhs.X, lhs.Y);
                const auto rhsDistance = distances.Get(rhs.X, rhs.Y);
                if (lhsDistance == rhsDistance) {
                    return EuclidDistance(lhs, giantPosition) < EuclidDistance(rhs, giantPosition);
                }
                return lhsDistance < rhsDistance;
            });
        } else {
            std::sort(allowedPositions.begin(), allowedPositions.end(), [&](const Point& lhs, const Point& rhs) {
                return ManhattanDistance(lhs, giantPosition) < ManhattanDistance(rhs, giantPosition);
            });
        }

        return allowedPositions.front();
    }

    bool HasAdjacentGiants(const Point& position) const {
        for (const auto& dir : GetPossibleDirections()) {
            const auto adjacentPosition = position + dir;
            if (!WorldMap.IsOnMap(adjacentPosition)) {
                continue;
            }
            if (WorldMap.HasGiant(adjacentPosition)) {
                return true;
            }
        }
        return false;
    }

    std::vector<Point> FindAllowedPositions() const {
        std::vector<Point> result;
        const auto playerPosition = Player.GetPosition();
        for (const auto& dir : GetPossibleDirections()) {
            const auto nextPosition = playerPosition + dir;
            if (!WorldMap.IsOnMap(nextPosition)) {
                continue;
            }
            if (!HasAdjacentGiants(nextPosition)) {
                result.push_back(nextPosition);
            }
        }
        return result;
    }

    const Giant& FindMostDistantGiant() const {
        size_t maxIdx = 0;
        int maxDistance = 0;
        for (size_t i = 0; i < Giants.size(); ++i) {
            const auto& giant = Giants[i];
            const int distanceToGiant = ManhattanDistance(giant.GetPosition(), Player.GetPosition());
            if (maxDistance < distanceToGiant) {
                maxDistance = distanceToGiant;
                maxIdx = i;
            }
        }
        return Giants[maxIdx];
    }

    DistanceMap FindDistancesToPoint(const Point& point) const {
        using VisitedMap = Matrix<uint8_t>;

        DistanceMap distances(WorldMap.GetMapWidth(), WorldMap.GetMapHeight(), INF);
        VisitedMap visited(WorldMap.GetMapWidth(), WorldMap.GetMapHeight(), false);
        std::queue<Point> toVisit;

        const auto setDistance = [&](const Point& position, int distance) {
            distances.Set(position.X, position.Y, distance);
            visited.Set(position.X, position.Y, true);
        };

        const auto findToVisit = [&, this](const Point& position) {
            for (const auto& candidateDir : GetPossibleDirections()) {
                const auto candidateToVisit = position + candidateDir;
                if (!WorldMap.IsOnMap(candidateToVisit) || visited.Get(candidateToVisit.X, candidateToVisit.Y)) {
                    continue;
                }
                setDistance(candidateToVisit, distances.Get(position.X, position.Y) + 1);
                if (!HasAdjacentGiants(candidateToVisit)) {
                    toVisit.push(candidateToVisit);
                }
            }
        };

        setDistance(point, 0);
        for (const auto& dir : GetPossibleDirections()) {
            const auto nextPosition = point + dir;
            if (!WorldMap.IsOnMap(nextPosition) || visited.Get(nextPosition.X, nextPosition.Y)) {
                continue;
            }
            setDistance(nextPosition, 1);
            findToVisit(nextPosition);
        }

        while (!toVisit.empty()) {
            const auto& next = toVisit.front();
            findToVisit(next);
            toVisit.pop();
        }

        return distances;
    }
};

std::unique_ptr<IStrategy> CreateMainStrategy(const GameWorldMap& worldMap, const Giant::ListType& giants, Thor& thor) {
    return std::make_unique<FollowMostDistant>(worldMap, giants, thor);
}
#pragma endregion

class World {
public:
    explicit World(std::istream& input)
        : Player(ReadThor(input))
        , Giants(ReadGiants(input))
        , WorldMap(MAX_MAP_X, MAX_MAP_Y)
        , Strategy(CreateMainStrategy(WorldMap, Giants, Player))
    {
    }

    void NextStep(std::istream& input, std::ostream& output) {
        FillWorldMap();
        DumpWorldMap(std::cerr);
        output << Strategy->MakeDecision() << std::endl;
        ClearWorldMap();

        Read<int>(input); // skip the remaining number of hammer strikes
        Giants = ReadGiants(input);
    }

    bool IsRunning() const {
        return true;
    }

private:
    static constexpr int THOR_STRIKE_RADIUS = 4;
    static constexpr int MAX_MAP_X = 40;
    static constexpr int MAX_MAP_Y = 18;

    Thor Player;
    Giant::ListType Giants;
    GameWorldMap WorldMap;
    std::unique_ptr<IStrategy> Strategy;

private:
    static Thor ReadThor(std::istream& input) {
        const auto position = Point::FromStream(input);
        const auto strikesLeft = Read<int>(input);
        return {position, THOR_STRIKE_RADIUS, strikesLeft};
    }

    static Giant::ListType ReadGiants(std::istream& input) {
        const int amount = Read<int>(input);
        Giant::ListType result;
        for (int i = 0; i < amount; ++i) {
            result.emplace_back(Point::FromStream(input));
        }
        return result;
    }

    void FillWorldMap() {
        WorldMap.PlaceThor(Player.GetPosition());
        for (const auto& giant : Giants) {
            WorldMap.PlaceGiant(giant.GetPosition());
        }
    }

    void ClearWorldMap() {
        WorldMap.Clear();
    }

    void DumpWorldMap(std::ostream& os) const {
        #ifdef PRINT_DEBUG_INFO
        static const auto renderEntity = [](const GameWorldMap::CellType& type) {
            switch (type) {
                case GameWorldMap::CellType::EMPTY: return '.';
                case GameWorldMap::CellType::THOR: return 'T';
                case GameWorldMap::CellType::GIANT: return 'G';
                default: return '?';
            }
        };

        os << "*" << std::string(2 * MAX_MAP_X - 1, '-') << "*\n|";
        for (int y = 0; y < MAX_MAP_Y; ++y) {
            bool isNewRow = y != 0;
            for (int x = 0; x < MAX_MAP_X; ++x) {
                if (isNewRow) {
                    os << "\n|";
                    isNewRow = false;
                }
                const auto entity = WorldMap.GetEntity({x, y});
                os << renderEntity(entity) << "|";
            }
        }
        os << "\n*" << std::string(2 * MAX_MAP_X - 1, '-') << "*" << std::endl;
        #endif
    }
};

int main(int argc, const char** argv) {
    try {
        World world(std::cin);
        while (world.IsRunning()) {
            world.NextStep(std::cin, std::cout);
        }
    } catch (const std::exception& exception) {
        std::cerr << "An error occurred: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}

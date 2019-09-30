#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

struct Point {
    int X = 0;
    int Y = 0;
};

class RectangularArea {
public:
    RectangularArea()
        : Empty(true)
    {}

    RectangularArea(int topLeftX, int topLeftY, int bottomRightX, int bottomRightY)
        : TopLeft{topLeftX, topLeftY}
        , BottomRight{bottomRightX, bottomRightY}
        , Empty(TopLeft.X > BottomRight.X || TopLeft.Y > BottomRight.Y)
    {
    }

    bool IsEmpty() const { return Empty; }

    Point GetMiddlePoint() const {
        int w = BottomRight.X - TopLeft.X;
        int h = BottomRight.Y - TopLeft.Y;
        return {TopLeft.X + w / 2, TopLeft.Y + h / 2};
    }

    // Intersection
    friend RectangularArea operator&(const RectangularArea& lhs, const RectangularArea& rhs) {
        if (lhs.Empty || rhs.Empty) {
            return { 0, 0, 0, 0 };
        }

        int newTopLeftX = std::max(lhs.TopLeft.X, rhs.TopLeft.X);
        int newTopLeftY = std::max(lhs.TopLeft.Y, rhs.TopLeft.Y);
        int newBottomRightX = std::min(lhs.BottomRight.X, rhs.BottomRight.X);
        int newBottomRightY = std::min(lhs.BottomRight.Y, rhs.BottomRight.Y);

        return {newTopLeftX, newTopLeftY, newBottomRightX, newBottomRightY};
    }

    // Union
    friend RectangularArea operator|(const RectangularArea& lhs, const RectangularArea& rhs) {
        if (lhs.Empty) {
            return rhs;
        } else if (rhs.Empty) {
            return lhs;
        }

        int newTopLeftX = std::min(lhs.TopLeft.X, rhs.TopLeft.X);
        int newTopLeftY = std::min(lhs.TopLeft.Y, rhs.TopLeft.Y);
        int newBottomRightX = std::max(lhs.BottomRight.X, rhs.BottomRight.X);
        int newBottomRightY = std::max(lhs.BottomRight.Y, rhs.BottomRight.Y);

        return { newTopLeftX, newTopLeftY, newBottomRightX, newBottomRightY };
    }

private:
    Point TopLeft;
    Point BottomRight;
    bool Empty;
};

struct Building {
    explicit Building(std::istream& is) {
        is >> Width >> Height;
    };

    int Width;
    int Height;
};

struct GameData {
    explicit GameData(std::istream& is) {
        is >> MaxTurns;
    }

    int MaxTurns;
};

class Batman {
public:
    explicit Batman(std::istream& is, const Building& house)
        : House(house)
    {
        is >> X >> Y;
    }

    int GetX() const { return X; }
    int GetY() const { return Y; }

    void JumpTo(const Point& location) {
        X = location.X;
        Y = location.Y;
    }

    RectangularArea DecidePossibleArea(char dir) const {
        switch (dir) {
            case 'U':
                return { X, -1, X, Y };
            case 'D':
                return { X, Y, X, House.Height };
            case 'L':
                return { -1, Y, X, Y };
            case 'R':
                return { X, Y, House.Width, Y };
            default:
                std::cerr << "Wrong bomb direction: " << dir << std::endl;
                return { -1, -1, -1, -1 };
        }
    }
    
private:
    const Building& House;
    int X;
    int Y;
};

class Game {
public:
    Game(std::istream& is)
        : House(is)
        , Data(is)
        , Player(is, House)
        , PossibleArea(0, 0, House.Width, House.Height)
    {}
        
    std::string DoStep(std::istream& is) {
        std::string bombDir;
        is >> bombDir;
        RunLogic(bombDir);
        return RenderOutput();
    }

private:
    const Building House;
    const GameData Data;
    Batman Player;
    RectangularArea PossibleArea;
    
    void RunLogic(const std::string& bombDir) {
        RectangularArea newBombArea;

        for (char dir : bombDir) {
            auto newArea = Player.DecidePossibleArea(dir);
            newBombArea = newBombArea | newArea;
        }

        PossibleArea = PossibleArea & newBombArea;
        Player.JumpTo(PossibleArea.GetMiddlePoint());
    }
    
    std::string RenderOutput() const {
        std::stringstream out;
        out << Player.GetX() << ' ' << Player.GetY();
        return out.str();
    }
};

int main()
{
    Game game(std::cin);
    while (true) {
        std::cout << game.DoStep(std::cin) << std::endl;
    }
}

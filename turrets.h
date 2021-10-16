#include <SDL2/SDL.h>
#include <vector>
#include <unordered_set>

struct Point
{
    int x;
    int y;

    bool operator==(const Point& p)
    {
        return (p.x == x) && (p.y == y);
    }
};

enum CellType
{
    EMPTY,
    WHITE,
    BLACK,
    TURRET_PREVIEW,
    ATTACKED_CELL,
    NUM_CELL_TYPES
};

class Turrets 
{

public:
    Turrets(int window_width, int window_height);   
    ~Turrets();
    bool Init();

    inline int GetOpponent(int side){ return (side == WHITE)?BLACK:WHITE; }

    void PrintGrid();
    void DisplayScore();
    void CalculateScore();

    bool InBounds(int x, int y);
    int NumNeighbours(int x, int y, int side);

    bool IsStarShape(int x, int y, int color);
    void FindStarShapes();

    void PlayMove(int mouse_x, int mouse_y);
    void ChangeSide();
    void ClearPreview();
    void Update();

    void Draw();
    void DrawGrid();

    inline bool ShouldWeQuit() { return quit; }

private:
    SDL_Window *window;
    SDL_Renderer *renderer;

    int window_height;
    int window_width;

    static const int kGridWidth = 16;
    static const int kGridHeight = 16;

    int cell_width;
    int cell_height;

    int grid[kGridHeight][kGridWidth] = { EMPTY };

    int side_to_move = WHITE;

    std::unordered_set<int> star_shapes_black;
    std::unordered_set<int> star_shapes_white;

    std::vector<Point> previewed_cells;
    Point attacked_square = {-1,-1};
    Point launch_square = {-1,-1};

    bool quit = false;
    bool preview_active = false;

    int occupied_white = 0;
    int occupied_black = 0;

    int used_additional_moves = 0;
};

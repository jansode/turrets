#include "turrets.h"
#include <iostream>
#include <algorithm>
#include <queue>

Turrets::Turrets(int window_width, int window_height):
    window_width(window_width), 
    window_height(window_height)
{}

Turrets::~Turrets()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Turrets::Init()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        return false;

    if(SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window, &renderer) < 0)
        return false;

    SDL_SetWindowTitle(window,"Turrets");

    cell_width = window_width / kGridWidth;
    cell_height = window_height / kGridHeight;
}

int Turrets::NumNeighbours(int x, int y, int side)
{
    int neighbours = 0;
    if(y < kGridHeight && grid[y+1][x] == side) ++neighbours;
    if(y > 0           && grid[y-1][x] == side) ++neighbours;
    if(x < kGridWidth  && grid[y][x+1] == side) ++neighbours;
    if(x > 0           && grid[y][x-1] == side) ++neighbours;

    return neighbours;
}

void Turrets::ClearPreview()
{
    for(int i = 0; i < previewed_cells.size(); ++i)
    {
        grid[previewed_cells[i].y][previewed_cells[i].x] = EMPTY;
    }

    if(attacked_square.x != -1)
    {
        grid[attacked_square.y][attacked_square.x] = GetOpponent(side_to_move);
    }

    previewed_cells.clear();
    preview_active = false;
    attacked_square = {-1,-1};
    launch_square = {-1,-1};
}

void Turrets::CalculateScore()
{
    occupied_white = 0;
    occupied_black = 0;

    for(int y = 0; y < kGridHeight; ++y)
    {
        for(int x = 0; x < kGridWidth; ++x)
        {
            switch(grid[y][x])
            {
                case WHITE: occupied_white++; break;
                case BLACK: occupied_black++; break;
                default: break;
            }
        }
    }
}

void Turrets::DisplayScore()
{
    std::cout<<"==Score==\n";
    std::cout<<"White: "<<occupied_white<<"\n";
    std::cout<<"Black: "<<occupied_black<<"\n\n";
}

void Turrets::ChangeSide()
{
    if(side_to_move == WHITE)
        side_to_move = BLACK;
    else
        side_to_move = WHITE;

    if(preview_active) 
        ClearPreview();

    DisplayScore();
}

void Turrets::PlayMove(int mouse_x, int mouse_y)
{
    int x = mouse_x / cell_width;
    int y = mouse_y / cell_height;

    if(preview_active)
    {
        if(launch_square.x == x && launch_square.y == y)
        {
            ClearPreview();
            return;
        }

        int opponent = GetOpponent(side_to_move);

        Point search_point = {x,y};
        if(search_point == attacked_square)
        {
            // Flood fill.
            std::queue<Point> queue;

            queue.push(attacked_square);

            while(!queue.empty())
            {
                Point current = queue.front();
                queue.pop();

                grid[current.y][current.x] = side_to_move;

                Point new_point = {-1,-1};
                if(current.y > 0 && grid[current.y-1][current.x] == opponent)    
                {
                    new_point.x = current.x;
                    new_point.y = current.y-1;
                    queue.push(new_point);
                }
                if(current.y < kGridHeight-1 && grid[current.y+1][current.x] == opponent)    
                {
                    new_point.x = current.x;
                    new_point.y = current.y+1;
                    queue.push(new_point);
                }
                if(current.x > 0 && grid[current.y][current.x-1] == opponent)    
                {
                    new_point.x = current.x-1;
                    new_point.y = current.y;
                    queue.push(new_point);
                }
                if(current.x < kGridWidth-1 && grid[current.y][current.x+1] == opponent)    
                {
                    new_point.x = current.x+1;
                    new_point.y = current.y;
                    queue.push(new_point);
                }
            }

            ChangeSide();

            if(launch_square.x != -1)
            {
                grid[launch_square.y][launch_square.x] = EMPTY;
            }

            CalculateScore();
            return;
        }

        auto found = std::find(previewed_cells.begin(), previewed_cells.end(), search_point);

        // Choose cell on which to perform turret attack.
        if(found != previewed_cells.end())
        {
            grid[launch_square.y][launch_square.x] = EMPTY;
            previewed_cells.erase(found);
            grid[y][x] = side_to_move;
        }

        CalculateScore();
        ChangeSide();
        return;
    }

    if(grid[y][x] == EMPTY)
    {
        if(side_to_move == WHITE)
            grid[y][x] = WHITE;
        else
            grid[y][x] = BLACK;

        CalculateScore();
        ChangeSide();
    }
    else if(grid[y][x] == side_to_move 
            && x != 0 && y != 0 
            && x != kGridWidth - 1 
            && y != kGridHeight - 1)
    {
        if(preview_active) 
            ClearPreview();

        // Check turret move
        if(NumNeighbours(x,y,side_to_move) == 3)
        {
            int opponent = GetOpponent(side_to_move);
            if(grid[y-1][x] == EMPTY || grid[y-1][x] == opponent)
            {
                // Shoot up.
                for(int i = 0;; ++i)
                {
                    if(grid[y-i-1][x] == opponent) 
                    {
                        attacked_square = {x,y-i-1};
                        grid[y-i-1][x] = ATTACKED_CELL;
                        break;
                    }
                    else if(grid[y-i-1][x] == side_to_move)
                        break;

                    previewed_cells.push_back({x,y-i-1});
                    grid[y-i-1][x] = TURRET_PREVIEW;

                    if(y-i-1 == 0) break;
                }
                
                launch_square = {x,y};
                preview_active = true;
            }
            else if(grid[y+1][x] == EMPTY || grid[y+1][x] == opponent)
            {
                // Shoot down.
                for(int i = 0;; ++i)
                {
                    if(grid[y+i+1][x] == opponent) 
                    {
                        attacked_square = {x,y+i+1};
                        grid[y+i+1][x] = ATTACKED_CELL;
                        break;
                    }
                    else if(grid[y+i+1][x] == side_to_move)
                        break;

                    previewed_cells.push_back({x,y+i+1});
                    grid[y+i+1][x] = TURRET_PREVIEW;

                    if(y+i == kGridHeight) break;
                }

                launch_square = {x,y};
                preview_active = true;
            }
            else if(grid[y][x+1] == EMPTY || grid[y][x+1] == opponent)
            {
                // Shoot right.
                for(int i = 0;; ++i)
                {
                    if(grid[y][x+i+1] == opponent) 
                    {
                        attacked_square = {x+i+1,y};
                        grid[y][x+i+1] = ATTACKED_CELL;
                        break;
                    }
                    else if(grid[y][x+i+1] == side_to_move)
                        break;

                    previewed_cells.push_back({x+i+1,y});
                    grid[y][x+i+1] = TURRET_PREVIEW;

                    if(x+i == kGridWidth) break;
                }

                launch_square = {x,y};
                preview_active = true;
            }
            else if(grid[y][x-1] == EMPTY || grid[y][x-1] == opponent)
            {
                // Shoot left.
                for(int i = 0;; ++i)
                {
                    if(grid[y][x-i-1] == opponent) 
                    {
                        attacked_square = {x-i-1,y};
                        grid[y][x-i-1] = ATTACKED_CELL;
                        break;
                    }
                    else if(grid[y][x-i-1] == side_to_move)
                        break;

                    previewed_cells.push_back({x-i-1,y});
                    grid[y][x-i-1] = TURRET_PREVIEW;

                    if(x-i-1 == 0) break;
                }

                launch_square = {x,y};
                preview_active = true;
            }
        }
    }
}

void Turrets::Update()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    int x,y;
                    SDL_GetMouseState(&x,&y);
                    PlayMove(x,y);
                }
                break;
            case SDL_QUIT:
                quit = true;
                break;
        }
    }

}

void Turrets::DrawGrid()
{
    SDL_Rect rect;
    rect.w = cell_width;
    rect.h = cell_height;
    for(int y = 0; y < kGridHeight; ++y)
    {
        for(int x = 0; x < kGridWidth; ++x)
        {
            rect.x = x*cell_width;
            rect.y = y*cell_height;

            switch(grid[y][x]) 
            {
                case EMPTY:
                    SDL_SetRenderDrawColor(renderer,128,128,128,255);
                    break;
                case WHITE:
                    SDL_SetRenderDrawColor(renderer,255,255,255,255);
                    break;
                case BLACK:
                    SDL_SetRenderDrawColor(renderer,0,0,0,255);
                    break;
                case TURRET_PREVIEW:
                    SDL_SetRenderDrawColor(renderer,0,255,0,255);
                    break;
                case ATTACKED_CELL:
                    SDL_SetRenderDrawColor(renderer,255,0,0,255);
                    break;
            }

            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0,0,0,255);

    for(int y = 0; y < kGridHeight; ++y)
        SDL_RenderDrawLine(renderer, 0, y*cell_height, cell_width*kGridWidth, y*cell_height);

    for(int x = 0; x < kGridWidth; ++x)
        SDL_RenderDrawLine(renderer, x*cell_width, 0, x*cell_width, cell_height*kGridHeight);
}

void Turrets::Draw()
{
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderClear(renderer);

    DrawGrid();

    SDL_RenderPresent(renderer);
}

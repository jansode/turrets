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

bool Turrets::InBounds(int x, int y)
{
    return (x >= 0 && x < kGridWidth && y >= 0 && y < kGridHeight);
}

bool Turrets::IsStarShape(int x, int y, int color)
{
    return InBounds(x-1,y) && grid[y][x-1] == color && 
           InBounds(x+1,y) && grid[y][x+1] == color &&
           InBounds(x,y+1) && grid[y+1][x] == color &&
           InBounds(x,y-1) && grid[y-1][x] == color &&
           grid[y+1][x+1] == EMPTY && 
           grid[y-1][x-1] == EMPTY &&
           grid[y+1][x-1] == EMPTY &&
           grid[y-1][x+1] == EMPTY;
}

void Turrets::FindStarShapes()
{
    for(int y=0; y<kGridHeight; ++y)
    {
        for(int x=0; x<kGridWidth; ++x)
        {
            switch(grid[y][x])
            {
                case WHITE:

                    if(IsStarShape(x,y, WHITE))
                        star_shapes_white.insert(y*kGridWidth+x);

                    break;
                case BLACK:

                    if(IsStarShape(x,y, BLACK))
                        star_shapes_black.insert(y*kGridWidth+x);

                    break;
            }
        }
    }
}

void Turrets::PrintGrid()
{
    std::cout<<std::endl;
    for(int y = 0; y<kGridHeight; ++y)
    {
        for(int x = 0; x <kGridWidth; ++x)
        {
            switch(grid[y][x])
            {
                case EMPTY: std::cout<<"0"; break;
                case WHITE: std::cout<<"1"; break;
                case BLACK: std::cout<<"2"; break;
            }
        }
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
}

void Turrets::DisplayScore()
{
    std::cout<<"==Score==\n";
    std::cout<<"White: "<<occupied_white<<"\n";
    std::cout<<"Black: "<<occupied_black<<"\n\n";

    std::cout<<"==Additional moves==\n";
    if(side_to_move == WHITE)
        std::cout<<star_shapes_white.size()-used_additional_moves<<std::endl;
    else
        std::cout<<star_shapes_black.size()-used_additional_moves<<std::endl;
}

void Turrets::ChangeSide()
{
    FindStarShapes(); 

    if(side_to_move == WHITE)
    {
        std::cout<<"Star shapes size: "<<star_shapes_white.size()<<std::endl;
        if(star_shapes_white.size()-used_additional_moves > 0)
        {
            std::cout<<"White star shape"<<std::endl;
            used_additional_moves++;
            return;
        }

        side_to_move = BLACK;
    }
    else
    {
        if(star_shapes_black.size()-used_additional_moves > 0)
        {
            std::cout<<"Black star shape"<<std::endl;
            used_additional_moves++;
            return;
        }

        side_to_move = WHITE;
    }

    used_additional_moves = 0;

    star_shapes_black.clear();
    star_shapes_white.clear();

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

            #ifdef PRINT_GRID
            PrintGrid();
            #endif

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

        #ifdef PRINT_GRID
        PrintGrid();
        #endif

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
        {
            ClearPreview();
            return;
        }

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
                        break;
                    }
                    else if(grid[y-i-1][x] == side_to_move)
                        break;

                    previewed_cells.push_back({x,y-i-1});

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
                        break;
                    }
                    else if(grid[y+i+1][x] == side_to_move)
                        break;

                    previewed_cells.push_back({x,y+i+1});

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
                        break;
                    }
                    else if(grid[y][x+i+1] == side_to_move)
                        break;

                    previewed_cells.push_back({x+i+1,y});

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
                        break;
                    }
                    else if(grid[y][x-i-1] == side_to_move)
                        break;

                    previewed_cells.push_back({x-i-1,y});

                    if(x-i-1 == 0) break;
                }

                launch_square = {x,y};
                preview_active = true;
            }
        }
    }

    #ifdef PRINT_GRID
    PrintGrid();
    #endif
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
            }

            SDL_RenderFillRect(renderer, &rect);
        }
    }

    for(int i=0; i<previewed_cells.size(); ++i)
    {
        Point p = previewed_cells[i];
        rect.x = p.x*cell_width;
        rect.y = p.y*cell_height;
        SDL_SetRenderDrawColor(renderer,0,255,0,255);
        SDL_RenderFillRect(renderer, &rect);
    }

    if(attacked_square.x != -1)
    {
        rect.x = attacked_square.x*cell_width;
        rect.y = attacked_square.y*cell_height;
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRect(renderer,&rect);
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

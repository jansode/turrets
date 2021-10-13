#include "turrets.h"

int main()
{
    Turrets t(600, 600);
    t.Init();

    while(!t.ShouldWeQuit())
    {
        t.Update();
        t.Draw();
    }


    return 0;
}

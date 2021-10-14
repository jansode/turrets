#include "turrets.h"

int main()
{
    Turrets t(512, 512);
    t.Init();

    while(!t.ShouldWeQuit())
    {
        t.Update();
        t.Draw();
    }


    return 0;
}

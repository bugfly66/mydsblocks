#include "../util.h"
#include "music.h"
#include <stdio.h>

#define MUSICINFOCMD            (char *[]) {"playerctl","metadata","--format={{title}} - {{artist}}",NULL}
#define MUSICSTATUSCMD          (char *[]) {"playerctl","status",NULL}
#define MUSICNEXT               (char *[]) {"playerctl","next",NULL}
#define MUSICPREV               (char *[]) {"playerctl","previous",NULL}        
#define MUSICTOGGLE             (char *[]) {"playerctl","play-pause",NULL}        
#define ICONplay           COL0"󰏤"NOR
#define ICONpause          WARN"󰐊"NOR
#define ICONoff                "" 
size_t
musicu(char* str,int signval){
    char buff1[56];
    char buff2[3];
    size_t infol,statl;
    char *icon = ICONoff;

    infol = getcmdout(MUSICINFOCMD,buff1,sizeof buff1 - 1);
    buff1[infol] = '\0';
    if(buff1[infol - 3] == '-')
        buff1[infol - 3 ] = '\0';   
    statl = getcmdout(MUSICSTATUSCMD,buff2,sizeof buff2 - 1);
    buff2[statl] = '\0';
    if(buff2[1] == 'l')
        icon = ICONplay;
    else if(buff2[1] == 'a')
        icon = ICONpause;

    return sprintf(str, "%s %s",icon,buff1);
}

void
musicc(int button)
{
    switch (button) {
            case 1:
                    cspawn(MUSICTOGGLE);
                    break;
            case 2:
                    cspawn(MUSICPREV); 
                    break;
            case 3:
                    cspawn(MUSICNEXT);
                    break;
    }
}


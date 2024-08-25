#include <stdio.h>
#include <string.h>

#include "../util.h"
#include "wifi.h"

#define ICONerr                          "󰤮 " NOR 

#define ICONw0                           "󰤫 " NOR 
#define ICONw1                           "󰤯 " NOR
#define ICONw2                           "󰤟 " NOR
#define ICONw3                           "󰤢 " NOR
#define ICONw4                           "󰤥 " NOR
#define ICONw5                           "󰤨 " NOR

#define WIFIFILE                        "/proc/net/wireless"
#define INTERFACENAME                   "wlp4s0"

/* use watch to determine apropiate levels on the proc file */
/* you are looking for value of Quality level */
/* should adjust the ICON() macro as well */
#define VERYLOWLEVEL                    -90
#define LOWLEVEL                        -70
#define MEDIUMLEVEL                     -50
#define HIGHLEVEL                       -30

#define ICON(level)                     icons[((level * -(LENGTH(icons)) + 30) / 90) -1]

/* Needs NetworkManager and st */
#define NM_CONNECT                      (char *[]){ "st", "-g", "50x32-1", "-i", "-e", "sh", "-c", "nmtui-connect", NULL }

size_t
wifiu(char *str, int sigval)
{
    FILE *fp;
    int val, quality_level = 0;
    char field_name[16];
    static char *icons[] = { ICONw5, ICONw4, ICONw3, ICONw2, ICONw1 };

    if (!(fp = fopen(WIFIFILE, "r"))) {
        strcpy(str, ICONerr);
        return sizeof ICONerr;
    }

    /* Skip n lines of the wireless file */
    for (int n=0; n<2; n++){
        if(fscanf(fp, "%*[^\n]\n") == -1)
        return SPRINTF(str,ERR  "%s", ICONerr);
    }
    if (fscanf(fp, "%[^:]s", field_name) != 1) {
            fclose(fp);
            SPRINTF(str, NOR ICONw0);
            return sizeof ICONw0;
    }
    if (fscanf(fp, "%*c %*d %*u. %d", &val) != 1) {
            fclose(fp);
            SPRINTF(str, NOR ICONw0);
            return sizeof ICONw0;
    }
    if (!quality_level && strncmp(field_name, INTERFACENAME, sizeof INTERFACENAME) == 0) {
        quality_level = val;
    }
    fclose(fp);
    if (quality_level < -100)
        return SPRINTF(str, WARN "%s%d", ICONw0, quality_level);
    return SPRINTF(str, NOR  "%s%d", ICON(quality_level), quality_level);
}

void
wific(int button)
{
    switch (button) {
        case 1:
                break;
        case 2:
                cspawn(NM_CONNECT);
                break;
        case 3:
                break;
    }

}

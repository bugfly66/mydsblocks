#include "shared.h"

#define NOR                             "\x11" /* default status color */
#define COL0                            "\x12" /* default icon color */
#define COL1                            "\x13" /* alternate icon color */
#define COL2                            "\x14" 
#define COL3                            "\x15" 
#define COL4                            "\x16" 
#define COL5                            "\x17" 
#define COL6                            "\x18" 
#define WARN                            "\x19"
#define ERR                             "\x1a"
#define SCRIPT(name)                    "/home/zx/scripts/" name
#define TERMCMD(...)                    cspawn((char *[]){ "st", "-e", __VA_ARGS__, NULL })

#define SPRINTF(str, ...)               ({ \
                                                int len = snprintf(str, BLOCKLENGTH, __VA_ARGS__); \
                                                len < BLOCKLENGTH ? len + 1 : BLOCKLENGTH; \
                                        })

void cspawn(char *const *arg);
void csigself(int sig, int sigval);
size_t getcmdout(char *const *arg, char *cmdout, size_t cmdoutlen);
int readint(const char *path, int *var);
void uspawn(char *const *arg);

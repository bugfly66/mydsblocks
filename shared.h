#include <X11/Xlib.h>
#include "sigvals.h"

#define BLOCKLENGTH                     80 

#define LENGTH(X)                       (sizeof X / sizeof X[0])

void cleanup(void);

extern pid_t pid;

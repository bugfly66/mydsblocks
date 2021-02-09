#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "shared.h"

#define STATUSLENGTH                    256
#define LOCKFILE                        "/tmp/dsblocks.pid"

#define DELIMITERLENGTH                 sizeof delimiter

typedef struct {
        void (*const funcu)(char *str, int sigval);
        void (*const funcc)(int button);
        const int interval;
        const int signal;
        char curtext[BLOCKLENGTH];
        char prvtext[BLOCKLENGTH];
} Block;

#include "blocks.h"

static void buttonhandler(int sig, siginfo_t *info, void *ucontext);
static void cleanup();
static void setroot();
static void setupsignals();
static void sighandler(int sig, siginfo_t *info, void *ucontext);
static void statusloop();
static void termhandler(int sig);
static int updatestatus();
static void writepid();

Display *dpy;
pid_t pid;

static char statustext[STATUSLENGTH + DELIMITERLENGTH];
static sigset_t blocksigmask;

void
buttonhandler(int sig, siginfo_t *info, void *ucontext)
{
        sig = info->si_value.sival_int >> 8;
        for (Block *block = blocks; block->funcu; block++)
                if (block->signal == sig)
                        switch (fork()) {
                                case -1:
                                        perror("buttonhandler - fork");
                                        break;
                                case 0:
                                        close(ConnectionNumber(dpy));
                                        block->funcc(info->si_value.sival_int & 0xff);
                                        exit(0);
                        }
}

void
cleanup()
{
        unlink(LOCKFILE);
        XStoreName(dpy, DefaultRootWindow(dpy), "");
        XCloseDisplay(dpy);
}

void
setroot()
{
        if (updatestatus()) {
                XStoreName(dpy, DefaultRootWindow(dpy), statustext);
                XSync(dpy, False);
        }
}

void
setupsignals()
{
        struct sigaction sa;

        /* populate blocksigmask */
        sigemptyset(&blocksigmask);
        sigaddset(&blocksigmask, SIGHUP);
        sigaddset(&blocksigmask, SIGINT);
        sigaddset(&blocksigmask, SIGTERM);
        for (Block *block = blocks; block->funcu; block++)
                if (block->signal > 0)
                        sigaddset(&blocksigmask, SIGRTMIN + block->signal);

        /* setup signal handlers */
        /* to handle HUP, INT and TERM */
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = termhandler;
        sigaction(SIGHUP, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        /* to ignore unused realtime signals */
        // sa.sa_flags = SA_RESTART;
        // sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_IGN;
        for (int i = SIGRTMIN + 1; i <= SIGRTMAX; i++)
                sigaction(i, &sa, NULL);

        /* to prevent forked children from becoming zombies */
        sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
        // sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_DFL;
        sigaction(SIGCHLD, &sa, NULL);

        /* to handle signals generated by dwm on click events */
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        // sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = buttonhandler;
        sigaction(SIGRTMIN, &sa, NULL);

        /* to handle update signals for individual blocks */
        sa.sa_flags |= SA_NODEFER;
        sa.sa_mask = blocksigmask;
        sa.sa_sigaction = sighandler;
        for (Block *block = blocks; block->funcu; block++)
                if (block->signal > 0)
                        sigaction(SIGRTMIN + block->signal, &sa, NULL);
}

void
sighandler(int sig, siginfo_t *info, void *ucontext)
{
        sig -= SIGRTMIN;
        for (Block *block = blocks; block->funcu; block++)
                if (block->signal == sig)
                        block->funcu(block->curtext, info->si_value.sival_int);
        setroot();
}

void
statusloop()
{
        int i;
        struct timespec t;

        /* first run */
        sigprocmask(SIG_BLOCK, &blocksigmask, NULL);
        for (Block *block = blocks; block->funcu; block++)
                if (block->interval >= 0)
                        block->funcu(block->curtext, NILL);
        setroot();
        sigprocmask(SIG_UNBLOCK, &blocksigmask, NULL);
        t.tv_sec = INTERVALs, t.tv_nsec = INTERVALn;
        while (nanosleep(&t, &t) == -1)
                if (errno != EINTR) {
                        perror("statusloop - nanosleep");
                        exit(1);
                }
        /* main loop */
        for (i = 1; ; i++) {
                sigprocmask(SIG_BLOCK, &blocksigmask, NULL);
                for (Block *block = blocks; block->funcu; block++)
                        if (block->interval > 0 && i % block->interval == 0)
                                block->funcu(block->curtext, NILL);
                setroot();
                sigprocmask(SIG_UNBLOCK, &blocksigmask, NULL);
                t.tv_sec = INTERVALs, t.tv_nsec = INTERVALn;
                while (nanosleep(&t, &t) == -1);
        }
}

void
termhandler(int sig)
{
        cleanup();
        exit(0);
}

/* returns whether block outputs have changed and updates statustext if they have */
int
updatestatus()
{
        char *s = statustext;
        char *c, *p;
        Block *block = blocks;

        /* checking half of the function */
        for (; block->funcu; block++) {
                c = block->curtext, p = block->prvtext;
                for (; *c != '\0' && *c == *p; c++, p++);
                s += c - block->curtext;
                if (*c != *p)
                        goto update;
                if (c == block->curtext)
                        continue;
                if (block->funcc /* && block->signal */)
                        s++;
                s += DELIMITERLENGTH;
        }
        return 0;

        /* updating half of the function */
        for (; block->funcu; block++) {
                c = block->curtext, p = block->prvtext;
update:
                for (; (*p = *c) != '\0'; c++, p++)
                        *(s++) = *c;
                if (c == block->curtext)
                        continue;
                if (block->funcc /* && block->signal */)
                        *(s++) = block->signal;
                memcpy(s, delimiter, DELIMITERLENGTH);
                s += DELIMITERLENGTH;
        }
        if (s != statustext)
                s -= DELIMITERLENGTH;
        *s = '\0';
        return 1;
}

void
writepid()
{
        int fd;
        struct flock fl;

        if ((fd = open(LOCKFILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
                perror("writepid - open");
                exit(1);
        }
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        if (fcntl(fd, F_SETLK, &fl) == -1) {
                if (errno == EACCES || errno == EAGAIN) {
                        fputs("Error: another instance of dsblocks is already running.\n", stderr);
                        exit(2);
                }
                perror("writepid - fcntl");
                exit(1);
        }
        if (ftruncate(fd, 0) == -1) {
                perror("writepid - ftruncate");
                exit(1);
        }
        if (dprintf(fd, "%ld", (long)pid) < 0) {
                perror("writepid - dprintf");
                exit(1);
        }
}

int
main(int argc, char *argv[])
{
        if (!(dpy = XOpenDisplay(NULL))) {
                fputs("Error: could not open display.\n", stderr);
                return 1;
        }
        pid = getpid();
        writepid();
        setupsignals();
        statusloop();
        cleanup();
        return 0;
}

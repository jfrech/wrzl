/* Jonathan Frech, 2021-03-24 */
/* wrzl - run a command with elevated privileges */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE (4096)
extern char **environ;


int sanitize_environ() {
    static char *filter[] = {
        "PATH", "COLORTERM", "EDITOR", "PWD", "SHELL", "TERM", NULL};
    static char buf[BUF_SIZE];

    for (char **env = environ; *env; ++env) {
        char **fil;
        for (fil = filter; *fil; ++fil) {
            size_t m = 0;
            while ((*env)[m] && (*fil)[m] == (*env)[m] && (*env)[m] != '=')
                ++m;
            if (!(*fil)[m] && (*env)[m] == '=')
                break;
        }
        if (*fil)
            continue;

        size_t m = 0;
        while ((*env)[m] && (*env)[m] != '=')
            ++m;
        if (m+1 > BUF_SIZE)
            return ENAMETOOLONG;
        memcpy(buf, *env, m);
        buf[m] = '\0';
        if (unsetenv(buf))
            return errno;
    }

    if (setenv("HOME", "/root", /*override=*/ true)
    ||  setenv("LOGNAME", "root", /*override=*/ true)
    ||  setenv("USER", "root", /*override=*/ true))
        return errno;

    return EXIT_SUCCESS;
}

#define ERR(...) \
    fprintf(stderr, "[wrzl] " __VA_ARGS__), fputc('\n', stderr), EXIT_FAILURE

int main(int argc, char **argv) {
    if (argc < 1)
        return ERR("failure: argc < 1");
    if (argc < 2)
        return ERR("usage: wrzl cmd [arg] ...");

    if (sanitize_environ())
        return ERR("failure: sanitize_environ");

    if (setgid(0) || setuid(0)) {
        if (errno == EPERM)
            return ERR("installed with insufficient privileges");
        return ERR("failure: setgid");
    }

    fprintf(stderr, "[wrzl] Really run \33[1m\33[94m");
    for (size_t j = 1; argv[j]; ++j) {
        for (char *arg = argv[j]; *arg; ++arg) {
            if (' ' < *arg && *arg != '\\' && *arg <= '~')
                putc(*arg, stderr);
            else
                fprintf(stderr, "\33[96m\\x%02x\33[94m", *arg);
        }
        if (argv[j+1])
            putc(' ', stderr);
    }
    fprintf(stderr, "\33[m with elevated privileges? [Y/n] ");
    char c = getc(stdin);
    if (c != '\n' && c != 'y' && c != 'Y')
        return ERR("aborting ...");


    execvp(argv[1], 1+argv);


    /* execve failed */
    if (errno == ENOENT)
        return ERR("command not found: %s", argv[1]);
    return ERR("execvp failure");
}

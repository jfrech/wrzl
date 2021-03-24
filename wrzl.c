/* Jonathan Frech, 2021-03-24 */
/* wrzl -- a minimalistic alternative to sudo, doas and asroot */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char **environ;


bool streq(char *s, char *z) {
    return *s ? *s == *z && streq(1+s, 1+z) : !*z; }

bool valid_environment_variable_name(char *name) {
    static char *FILTER[] = {
        "PATH", "COLORTERM", "EDITOR", "PWD", "SHELL", "TERM",
    NULL, };

    for (size_t j = 0; FILTER[j]; ++j)
        if (streq(name, FILTER[j]))
            return true;
    return false;
}

int sanitize_environ() {
    char **env = environ;

    size_t N = 32;
    char *name = malloc(N * sizeof *name);
    if (!name)
        return errno;

    for (char *name_val; (name_val = *env); ++env) {
        size_t n;
        for (n = 0; name_val[n] && name_val[n] != '='; ++n)
            ;

        if (n+1 > N) {
            N = n+1 + 32;
            char *name_grown = realloc(name, N * sizeof *name_grown);
            if (!name_grown)
                return free(name), errno;
            name = name_grown;
        }

        for (size_t j = 0; j < n; j++)
            name[j] = name_val[j];
        name[n] = '\0';
        /* char *val = n+1+name_val; */

        if (!valid_environment_variable_name(name)) {
            if (unsetenv(name))
                return free(name), errno;
        }
    }

    setenv("HOME", "/root", /*override=*/ true);
    setenv("LOGNAME", "root", /*override=*/ true);
    setenv("USER", "root", /*override=*/ true);

    return free(name), EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc < 1)
        return fprintf(stderr, "[wrzl] failure: argc < 1\n"), EXIT_FAILURE;
    if (argc < 2)
        return fprintf(stderr, "[wrzl] usage: wrzl cmd [arg] ...\n"), EXIT_FAILURE;

    if (sanitize_environ())
        return fprintf(stderr, "[wrzl] failure: sanitize_environ\n"), EXIT_FAILURE;

    if (setgid(0)) {
        if (errno == EPERM)
            return fprintf(stderr, "[wrzl] installed with insufficient privileges\n"), EXIT_FAILURE;
        return fprintf(stderr, "[wrzl] failure: setgid\n"), EXIT_FAILURE;
    }
    if (setuid(0)) {
        if (errno == EPERM)
            return fprintf(stderr, "[wrzl] installed with insufficient privileges\n"), EXIT_FAILURE;
        return fprintf(stderr, "[wrzl] failure: setuid\n"), EXIT_FAILURE;
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
    fprintf(stderr, "\33[m with elevated privileges? [y/N] ");
    char c = getc(stdin);
    if (c != '\n' && c != 'y' && c != 'Y') {
        return fprintf(stderr, "[wrzl] aborting ...\n"), EXIT_FAILURE;
    }


    execvp(argv[1], 1+argv);


    /* execve failed */
    if (errno == ENOENT)
        return fprintf(stderr, "[wrzl] command not found: %s\n", argv[1]), EXIT_FAILURE;
    return fprintf(stderr, "[wrzl] execvp failure\n"), EXIT_FAILURE;
}

/* Jonathan Frech, 2021-03-24 */
/* wrzl -- a minimalistic alternative to sudo, doas and asroot. */
/* Heavily inspired by asroot (see https://github.com/maandree/asroot). */

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
        size_t n = 0;
        for (; name_val[n] && name_val[n] != '=';)
            ++n;

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

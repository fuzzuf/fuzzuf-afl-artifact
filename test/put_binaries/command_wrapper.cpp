#include <cstdio>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s (command) (arguments passed to command...)");
        return 0;
    }
    
    execvp(argv[1], &argv[1]);
}

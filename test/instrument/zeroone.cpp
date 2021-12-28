#include <cstdio>

#define IFBRANCH(buf, idx, var) \
    if (buf[idx] == '1') { \
        var = 1; \
    } else { \
        var = 0; \
    }

int main() {
    char buf[256];
    fgets(buf, sizeof(buf), stdin);

    volatile int a, b, c, d, e, f, g, h;

    // do not put this into loop because we want to see control flow graph
    IFBRANCH(buf, 0, a);
    IFBRANCH(buf, 1, b);
    IFBRANCH(buf, 2, c);
    IFBRANCH(buf, 3, d);
    IFBRANCH(buf, 4, e);
    IFBRANCH(buf, 5, f);
    IFBRANCH(buf, 6, g);
    IFBRANCH(buf, 7, h);

    printf("Result: %d%d%d%d%d%d%d%d\n", a, b, c, d, e, f, g, h);
}

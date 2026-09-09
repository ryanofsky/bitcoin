// Child program launched by the probe. Prints its argv, appends the same to the
// marker file named by the PROBE_MARKER environment variable (so a launch is
// detected even when stdout is lost), and exits with code 37.
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    printf("CHILD argc=%d", argc);
    for (int i = 0; i < argc; ++i) printf(" [%s]", argv[i]);
    printf("\n");
    fflush(stdout);
    const char* marker = getenv("PROBE_MARKER");
    if (marker) {
        FILE* f = fopen(marker, "a");
        if (f) {
            fprintf(f, "argc=%d", argc);
            for (int i = 0; i < argc; ++i) fprintf(f, " [%s]", argv[i]);
            fprintf(f, "\n");
            fclose(f);
        }
    }
    return 37;
}

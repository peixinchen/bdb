#include <debugger.hh>
#include <exception.hh>
#include <cstring>
#include <libgen.h>


int main(int argc, const char *argv[]) {
    if (argc < 2) {
        auto argv0 = strdup(argv[0]);
        fprintf(stderr, "usage: %s <program>\n", basename(argv0));
        exit(EXIT_FAILURE);
    }

    BitTech::Debugger debugger{argv[1]};
    try {
        debugger.run();
    } catch (BitTech::exception const& exc) {
        printf("%s: %d: %s\n", exc.file.c_str(), exc.line, exc.reason.c_str());
    }
}

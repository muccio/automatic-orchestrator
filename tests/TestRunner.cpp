#include "TestHarness.h"
#include <string>

int main(int argc, char** argv) {
    std::string filter = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--test=", 0) == 0) {
            filter = arg.substr(7);
        } else if (arg == "--all") {
            filter = "";
        } else {
            filter = arg;
        }
    }

    return OrchestratorTest::TestRegistry::instance().run(filter);
}

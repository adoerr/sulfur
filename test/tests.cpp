#include <catch2/catch_test_macros.hpp>
#include <libsulfur/process.hpp>
#include <libsulfur/error.hpp>
#include <sys/types.h>
#include <csignal>

using namespace sulfur;

namespace {
    bool process_exists(pid_t pid) {
        return kill(pid, 0) == 0 || errno != ESRCH;
    }
}

TEST_CASE("process::launch success", "[process]") {
    const auto proc = process::launch("yes");
    REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch failure", "[process]") {
    REQUIRE_THROWS_AS(process::launch("/nonexistent/path/to/executable"), error);
}

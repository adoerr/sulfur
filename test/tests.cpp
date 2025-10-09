#include <catch2/catch_test_macros.hpp>
#include <libsulfur/process.hpp>
#include <libsulfur/error.hpp>
#include <sys/types.h>
#include <csignal>
#include <fstream>

using namespace sulfur;

namespace {
    bool process_exists(pid_t pid) {
        return kill(pid, 0) == 0 || errno != ESRCH;
    }

    char process_status(const pid_t pid) {
        std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");

        std::string line;
        std::getline(stat, line);
        // get current process state (3rd field in /proc/[pid]/stat)
        const size_t idx_stat = line.rfind(')') + 2;
        return line[idx_stat];
    }

    TEST_CASE("process::launch success", "[process]") {
        const auto proc = process::launch("yes");
        REQUIRE(process_exists(proc->pid()));
    }

    TEST_CASE("process::launch failure", "[process]") {
        REQUIRE_THROWS_AS(process::launch("/nonexistent/path/to/executable"), error);
    }

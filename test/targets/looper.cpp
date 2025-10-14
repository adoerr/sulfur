[[noreturn]] int main() {
    // don't use empty loop in order to prevent UB
    while (true) volatile int i = 42;
}
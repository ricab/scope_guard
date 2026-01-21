#include <iostream>

#include <scope_guard.hpp>

int main() {
    auto guard = sg::make_scope_guard([]() noexcept { return; });
}

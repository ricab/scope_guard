from conans import ConanFile


class ScopeguardConan(ConanFile):
    name = "scope_guard"
    version = "0.2.4-dev.1"
    license = "The Unlicense"
    author = "Ricardo Abreu ricab@ricabhome.org"
    url = "https://github.com/Lawrencemm/scope_guard"
    description = (
        "A modern C++ scope guard that is easy to use but hard to misuse. "
        "https://ricab.github.io/scope_guard/"
    )
    topics = ("scope guard", "RAII", "resource management")
    exports_sources = "scope_guard.hpp"
    no_copy_source = True

    def package(self):
        self.copy("*scope_guard.hpp", dst="include")

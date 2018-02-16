#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

namespace
{
  //////////////////////////////////////////////////////////////////////////////
  bool f_called = false;

  //////////////////////////////////////////////////////////////////////////////
  void f()
  {
    f_called = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Plain function")
{
  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard(f);
    REQUIRE_FALSE(f_called);
  }
  REQUIRE(f_called);
}

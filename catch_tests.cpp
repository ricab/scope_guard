#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

// TODO clarify test names with "guard"
// TODO homogenize newlines
// TODO separate into construction and call tests
// TODO replace booleans with counts
// TODO replace auto with const auto where possible
// TODO add custom functor tests
// TODO add member function tests
// TODO add actual exception test
// TODO add actual rollback test
// TODO add temporary test
// TODO add new/delete tests
// TODO add unique_ptr tests
// TODO add shared_ptr tests
// TODO add move into function tests
// TODO add move into container tests
// TODO add tests for descending guard
// TODO add tests for required noexcept
// TODO add tests for no implicitly ignored return (and btw, make sure it would be implicitly ignored)

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool f_called = false;
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

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Simple std::function")
{
  f_called = false;

  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard(std::function<decltype(f)>(f));
    REQUIRE_FALSE(f_called);
  }

  REQUIRE(f_called);
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool lambda_no_capture_called = false;
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function with no capture")
{
  {
    REQUIRE_FALSE(lambda_no_capture_called);
    auto guard = make_scope_guard([](){lambda_no_capture_called = true;});
    REQUIRE_FALSE(lambda_no_capture_called);
  }

  REQUIRE(lambda_no_capture_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function with capture")
{
  bool lambda_called = false;

  {
    auto guard = make_scope_guard([&lambda_called](){lambda_called=true;});
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function calling regular function")
{
  f_called = false;
  bool lambda_called = false;

  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  void negate_f(bool& b)
  {
    b = !b;
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test binded function")
{
  bool binded_called = false;
  {
    auto guard = make_scope_guard(std::bind(negate_f, std::ref(binded_called)));
    REQUIRE_FALSE(binded_called);
  }
  REQUIRE(binded_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test binded lambda")
{
  bool binded_called = false;
  {
    auto negate_lambda = [](bool& b){b = !b;};
    auto guard = make_scope_guard(std::bind(negate_lambda,
                                            std::ref(binded_called)));
    REQUIRE_FALSE(binded_called);
  }
  REQUIRE(binded_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test redundant guards")
{
  f_called = false;
  bool lambda_called = false;

  {
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g1 = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g2 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);

  auto g3 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
  REQUIRE(f_called);
  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test nested scopes")
{
  bool lvl0_called = false;
  bool lvl1_called = false;
  bool lvl2a_called = false;
  bool lvl2b_called = false;
  bool lvl3a_called = false;
  bool lvl3b_called = false;
  bool lvl3c_called = false;

  // TODO replace with binds
  auto lvl0_guard = make_scope_guard([&lvl0_called](){lvl0_called = true;});
  REQUIRE_FALSE(lvl0_called);

  {
    auto lvl1_guard = make_scope_guard([&lvl1_called](){lvl1_called = true;});
    REQUIRE_FALSE(lvl1_called);

    {
      auto lvl2a_guard =
          make_scope_guard([&lvl2a_called](){lvl2a_called = true;});
      REQUIRE_FALSE(lvl2a_called);

      {
        auto lvl3a_guard =
          make_scope_guard([&lvl3a_called](){lvl3a_called = true;});
        REQUIRE_FALSE(lvl3a_called);
      }

      REQUIRE(lvl3a_called);
      REQUIRE_FALSE(lvl2a_called);
    }

    REQUIRE(lvl2a_called);
    REQUIRE_FALSE(lvl1_called);
    REQUIRE_FALSE(lvl0_called);

    {
      auto lvl2b_guard =
          make_scope_guard([&lvl2b_called](){lvl2b_called = true;});
      REQUIRE_FALSE(lvl2b_called);

      {
        auto lvl3b_guard =
            make_scope_guard([&lvl3b_called](){lvl3b_called = true;});
        REQUIRE_FALSE(lvl3b_called);

        auto lvl3c_guard =
            make_scope_guard([&lvl3c_called](){lvl3c_called = true;});
        REQUIRE_FALSE(lvl3c_called);
      }

      REQUIRE(lvl3b_called);
      REQUIRE(lvl3c_called);
      REQUIRE_FALSE(lvl2b_called);

    }

    REQUIRE(lvl2b_called);
    REQUIRE_FALSE(lvl1_called);
    REQUIRE_FALSE(lvl0_called);

  }

  REQUIRE(lvl1_called);
  REQUIRE(lvl2a_called);
  REQUIRE(lvl2b_called);
  REQUIRE(lvl3a_called);
  REQUIRE(lvl3b_called);
  REQUIRE(lvl3c_called);
  REQUIRE_FALSE(lvl0_called);

}

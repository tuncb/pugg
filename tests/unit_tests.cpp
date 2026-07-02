#include <catch2/catch_test_macros.hpp>

#include <pugg/Kernel.h>

// ── Minimal in-process driver/server for unit testing ──────────────────────

struct Widget
{
  virtual ~Widget() = default;
  virtual std::string label() const = 0;
  static int version;
  static std::string server_name() { return "WidgetServer"; }
};
int Widget::version = 2;

class WidgetDriver : public pugg::Driver
{
public:
  WidgetDriver(std::string name, int ver)
      : pugg::Driver(Widget::server_name(), std::move(name), ver)
  {
  }
  virtual Widget *create() = 0;
};

class RedWidgetDriver : public WidgetDriver
{
public:
  RedWidgetDriver() : WidgetDriver("RedWidgetDriver", Widget::version) {}
  Widget *create() override
  {
    struct Red : Widget { std::string label() const override { return "Red"; } };
    return new Red{};
  }
};

class BlueWidgetDriver : public WidgetDriver
{
public:
  BlueWidgetDriver() : WidgetDriver("BlueWidgetDriver", Widget::version) {}
  Widget *create() override
  {
    struct Blue : Widget { std::string label() const override { return "Blue"; } };
    return new Blue{};
  }
};

class OldWidgetDriver : public WidgetDriver
{
public:
  OldWidgetDriver() : WidgetDriver("OldWidgetDriver", 1 /* old version */) {}
  Widget *create() override { return nullptr; }
};

// A completely unrelated driver type to test dynamic_cast safety
class ForeignDriver : public pugg::Driver
{
public:
  ForeignDriver() : pugg::Driver("OtherServer", "ForeignDriver", 1) {}
};

// ── Driver tests ────────────────────────────────────────────────────────────

TEST_CASE("Driver stores metadata correctly", "[driver]")
{
  RedWidgetDriver d;
  CHECK(d.name() == "RedWidgetDriver");
  CHECK(d.server_name() == Widget::server_name());
  CHECK(d.version() == Widget::version);
}

// ── Kernel::add_server ───────────────────────────────────────────────────────

TEST_CASE("add_server registers a named server", "[kernel][server]")
{
  pugg::Kernel kernel;
  kernel.add_server(Widget::server_name(), Widget::version);

  // A driver targeting that server should now be accepted
  auto accepted = kernel.add_driver(std::make_unique<RedWidgetDriver>());
  CHECK(accepted);
}

TEST_CASE("add_server template overload extracts name and version from type", "[kernel][server]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  auto accepted = kernel.add_driver(std::make_unique<RedWidgetDriver>());
  CHECK(accepted);
}

TEST_CASE("add_server overwrites a previously registered server", "[kernel][server]")
{
  pugg::Kernel kernel;
  kernel.add_server(Widget::server_name(), Widget::version);
  kernel.add_server(Widget::server_name(), Widget::version + 1);

  // Old min-version bumped: original driver (version 2) is now too old
  auto accepted = kernel.add_driver(std::make_unique<RedWidgetDriver>());
  CHECK_FALSE(accepted);
}

// ── Kernel::add_driver ───────────────────────────────────────────────────────

TEST_CASE("add_driver raw-pointer overload accepts and takes ownership", "[kernel][driver][compat]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(new RedWidgetDriver()));
  auto v = kernel.get_all_drivers<WidgetDriver>(Widget::server_name());
  CHECK(v.size() == 1);
}

TEST_CASE("add_driver raw-pointer overload deletes driver on failure", "[kernel][driver][compat]")
{
  // No server registered: add_driver must return false and delete the driver.
  // Running under ASan or Valgrind will catch a leak if deletion doesn't happen.
  pugg::Kernel kernel;
  CHECK_FALSE(kernel.add_driver(new RedWidgetDriver()));
}

TEST_CASE("add_driver rejects null pointer", "[kernel][driver]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  CHECK_FALSE(kernel.add_driver(nullptr));
}

TEST_CASE("add_driver rejects driver whose server is not registered", "[kernel][driver]")
{
  pugg::Kernel kernel;
  // No add_server call
  CHECK_FALSE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
}

TEST_CASE("add_driver rejects driver with version below minimum", "[kernel][driver]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>(); // min version == Widget::version == 2
  CHECK_FALSE(kernel.add_driver(std::make_unique<OldWidgetDriver>())); // version 1
}

TEST_CASE("add_driver accepts driver with exact minimum version", "[kernel][driver]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  CHECK(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
}

TEST_CASE("add_driver rejects duplicate driver name", "[kernel][driver]")
{
  // Replacing would destroy the first driver while callers may still hold
  // pointers to it, so a duplicate name must be rejected instead.
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  CHECK(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
  auto second = kernel.add_driver(std::make_unique<RedWidgetDriver>());
  CHECK_FALSE(second);
  CHECK(second.reason == pugg::AddDriverResult::Reason::duplicate_name);
  // The first registration is untouched
  auto all = kernel.get_all_drivers<WidgetDriver>(Widget::server_name());
  CHECK(all.size() == 1);
}

TEST_CASE("add_driver reports the failure reason and last_error", "[kernel][driver]")
{
  using Reason = pugg::AddDriverResult::Reason;
  pugg::Kernel kernel;

  CHECK(kernel.add_driver(nullptr).reason == Reason::null_driver);

  CHECK(kernel.add_driver(std::make_unique<RedWidgetDriver>()).reason ==
        Reason::unknown_server);
  CHECK_FALSE(kernel.last_error().empty());

  kernel.add_server<Widget>();
  CHECK(kernel.add_driver(std::make_unique<OldWidgetDriver>()).reason ==
        Reason::version_mismatch);
  CHECK_FALSE(kernel.last_error().empty());

  CHECK(kernel.add_driver(std::make_unique<RedWidgetDriver>()).reason == Reason::ok);
}

// ── Kernel::get_driver ───────────────────────────────────────────────────────

TEST_CASE("get_driver returns registered driver", "[kernel][get]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));

  auto *d = kernel.get_driver<WidgetDriver>(Widget::server_name(), "RedWidgetDriver");
  REQUIRE(d != nullptr);
  CHECK(d->name() == "RedWidgetDriver");
}

TEST_CASE("get_driver returns nullptr for unknown server", "[kernel][get]")
{
  pugg::Kernel kernel;
  auto *d = kernel.get_driver<WidgetDriver>("NoSuchServer", "RedWidgetDriver");
  CHECK(d == nullptr);
}

TEST_CASE("get_driver returns nullptr for unknown driver name", "[kernel][get]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));

  auto *d = kernel.get_driver<WidgetDriver>(Widget::server_name(), "NoSuchDriver");
  CHECK(d == nullptr);
}

TEST_CASE("get_driver returns nullptr on incompatible type cast (dynamic_cast safety)", "[kernel][get][safety]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));

  // Requesting a concrete type that is NOT the actual driver type must return nullptr,
  // not invoke undefined behaviour via static_cast.
  auto *wrong = kernel.get_driver<BlueWidgetDriver>(Widget::server_name(), "RedWidgetDriver");
  CHECK(wrong == nullptr);
}

// ── Kernel::get_all_drivers ──────────────────────────────────────────────────

TEST_CASE("get_all_drivers returns empty vector for unknown server", "[kernel][get_all]")
{
  pugg::Kernel kernel;
  auto v = kernel.get_all_drivers<WidgetDriver>("NoSuchServer");
  CHECK(v.empty());
}

TEST_CASE("get_all_drivers returns all registered drivers", "[kernel][get_all]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
  REQUIRE(kernel.add_driver(std::make_unique<BlueWidgetDriver>()));

  auto v = kernel.get_all_drivers<WidgetDriver>(Widget::server_name());
  CHECK(v.size() == 2);
}

TEST_CASE("get_all_drivers filters out drivers that fail dynamic_cast", "[kernel][get_all][safety]")
{
  pugg::Kernel kernel;
  // Register two different servers so we can add a foreign driver
  kernel.add_server<Widget>();
  kernel.add_server("OtherServer", 1);
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
  REQUIRE(kernel.add_driver(std::make_unique<BlueWidgetDriver>()));

  // get_all_drivers<BlueWidgetDriver> should only return the exact Blue one
  auto v = kernel.get_all_drivers<BlueWidgetDriver>(Widget::server_name());
  REQUIRE(v.size() == 1);
  CHECK(v[0]->name() == "BlueWidgetDriver");
}

// ── Kernel::clear_drivers ────────────────────────────────────────────────────

TEST_CASE("clear_drivers removes all drivers but keeps servers", "[kernel][clear]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
  REQUIRE(kernel.add_driver(std::make_unique<BlueWidgetDriver>()));

  kernel.clear_drivers();

  auto v = kernel.get_all_drivers<WidgetDriver>(Widget::server_name());
  CHECK(v.empty());

  // Server still exists: a new driver should be accepted without re-registering
  CHECK(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
}

// ── Kernel::clear ────────────────────────────────────────────────────────────

TEST_CASE("clear removes servers and drivers", "[kernel][clear]")
{
  pugg::Kernel kernel;
  kernel.add_server<Widget>();
  REQUIRE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));

  kernel.clear();

  // After clear, server is gone: add_driver must fail
  CHECK_FALSE(kernel.add_driver(std::make_unique<RedWidgetDriver>()));
}

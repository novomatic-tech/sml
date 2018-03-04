#include <boost/sml.hpp>
#include <array>
#include <cstdio>

namespace sml = boost::sml;

class Sender {
public:
  template<class TAddress, class TData>
  auto send(const TAddress& ip, const TData& data) {
    std::puts(ip);
    std::puts(data);
    return true;
  }
};

class Context {
public:
  const char* const ip{};
  bool valid(int id) const { return true; }
  void log(...) {}
};

struct connect {};
struct ping { int id{}; };
struct established {};
struct timeout {};
struct disconnect {};

const auto close = []{ std::puts("close"); };
const auto resetTimeout = [] { std::puts("resetTimeout"); };

const auto is_valid = [](const Context& ctx, const auto& event) {
  return ctx.valid(event.id);
};

auto establish =
  [max_retries = 10](const auto& address) mutable {
    return [&](Sender& sender, Context& ctx) {
      max_retries -= sender.send(address, "establish");

      if (!max_retries) {
        ctx.log("Can't send request!");
        max_retries = {};
      }
    };
  };

struct Connection {
  const char* const address{}; // local dependency injected

  auto operator()() {
    using namespace sml;
    return make_transition_table(
      * "Disconnected"_s + event<connect> / establish(address)       = "Connecting"_s,
        "Connecting"_s   + event<established>                        = "Connected"_s,
        "Connected"_s    + event<ping> [ is_valid ] / resetTimeout,
        "Connected"_s    + event<timeout> / establish(address)       = "Connecting"_s,
        "Connected"_s    + event<disconnect> / close                 = "Disconnected"_s
    );
  }
};

int main() {
  Context ctx{};
  Sender sender{};

  std::array<sml::sm<Connection>, 4> connections = {
    sml::sm<Connection>{Connection{"127.0.0.1"}, ctx, sender},
    sml::sm<Connection>{Connection{"127.0.0.2"}, ctx, sender},
    sml::sm<Connection>{Connection{"127.0.0.3"}, ctx, sender},
    sml::sm<Connection>{Connection{"127.0.0.4"}, ctx, sender}
  };

  connections[0].process_event(connect{});
  connections[1].process_event(established{});
  connections[2].process_event(ping{42});
  connections[3].process_event(disconnect{});
}
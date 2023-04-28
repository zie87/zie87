
#include <any>

#include <cstdio>

namespace detail {

template <bool ENABLE_V = false>
struct logger {
    static void write([[maybe_unused]] const char* message) noexcept {};
};

template <>
struct logger<true> {
    static void write([[maybe_unused]] const char* message) noexcept { std::printf("%s\n", message); }
};
}  // namespace detail

inline void log(const char* msg) noexcept {
    static constexpr bool enable_log = false;
    detail::logger<enable_log>::write(msg);
}

///////////////////////////////////////////////////////////////////

struct queue_interface {
    virtual ~queue_interface()             = default;
    virtual void push(const std::any& evt) = 0;
};

///////////////////////////////////////////////////////////////////

struct queue : queue_interface {
    void push([[maybe_unused]] const std::any&) override { log("push push push!"); }
};

void push_to(queue& q, const std::any& evt) { q.push(evt); }

struct communication {
    communication(queue_interface& sink) noexcept : m_sink(sink) {}

    void do_something() {
        log("communication!");
        std::any msg(42);
        m_sink.push(msg);
    }

private:
    queue_interface& m_sink;
};

struct motorization {
    motorization(queue_interface& sink) noexcept : m_sink(sink) {}

    void do_something() {
        log("motorization!");
        std::any msg(17.f);
        m_sink.push(msg);
    }

private:
    queue_interface& m_sink;
};

struct executer {
    void do_something() {
        log("executer!");
        m_com.do_something();
        m_mot.do_something();
        m_mot.do_something();
        m_com.do_something();
    }

    queue m_queue{};
    communication m_com{m_queue};
    motorization m_mot{m_queue};
};

#include "ubench.h"


UBENCH_EX(sink, interface_sink) {
    executer ex{};

  UBENCH_DO_BENCHMARK() {
      ex.do_something();
  }

}
UBENCH_MAIN();

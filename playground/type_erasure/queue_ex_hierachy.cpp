
#include <any>
#include <memory>

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

struct queue {
    void push([[maybe_unused]] const std::any&) { log("push push push!"); }
};

void push_to(queue& q, const std::any& evt) { q.push(evt); }

struct communication_sink_interface {
    virtual void push(int)                  = 0;
    virtual ~communication_sink_interface() = default;
};

struct communication {
    using ptr_type = std::unique_ptr<communication_sink_interface>;
    communication(ptr_type&& sink) noexcept : m_sink(std::move(sink)) {}

    void do_something() {
        log("communication!");
        m_sink->push(42);
    }

private:
    ptr_type m_sink;
};

struct motorization_sink_interface {
    virtual void push(float)               = 0;
    virtual ~motorization_sink_interface() = default;
};

struct motorization {
    using ptr_type = std::unique_ptr<motorization_sink_interface>;

    motorization(ptr_type&& sink) noexcept : m_sink(std::move(sink)) {}

    void do_something() {
        log("motorization!");
        m_sink->push(17.f);
    }

private:
    ptr_type m_sink;
};

struct executer {
    struct com_sink : communication_sink_interface {
        com_sink(queue& q) noexcept : m_queue(q) {}

        void push(int val) override {
            std::any evt{val};
            m_queue.push(evt);
        }

        queue& m_queue;
    };

    struct mot_sink : motorization_sink_interface {
        mot_sink(queue& q) noexcept : m_queue(q) {}

        void push(float val) override {
            std::any evt{val};
            m_queue.push(evt);
        }

        queue& m_queue;
    };

    void do_something() {
        log("executer!");
        m_com.do_something();
        m_mot.do_something();
        m_mot.do_something();
        m_com.do_something();
    }

    queue m_queue{};
    communication m_com{std::make_unique<com_sink>(m_queue)};
    motorization m_mot{std::make_unique<mot_sink>(m_queue)};
};

#include "ubench.h"


UBENCH_EX(sink, external_sink) {
    executer ex{};

  UBENCH_DO_BENCHMARK() {
      ex.do_something();
  }

}
UBENCH_MAIN();

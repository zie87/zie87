
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
    static constexpr bool enable_log = true;
    detail::logger<enable_log>::write(msg);
}

///////////////////////////////////////////////////////////////////

struct tag {};

class queue_ref {
public:
    queue_ref(const queue_ref& other) noexcept = default;
    queue_ref(queue_ref&& other) noexcept      = default;

    template <typename T>
    queue_ref(tag, T& obj) noexcept
        : m_object_ptr{std::addressof(obj)}
        , m_do{[](void* ptr, const std::any& evt) { push_to(*static_cast<T*>(ptr), evt); }} {}

    void push(const auto& msg) {
        std::any evt{msg};
        m_do(m_object_ptr, evt);
    }

private:
    using operator_type = void(void*, const std::any&);
    void* m_object_ptr{nullptr};
    operator_type* m_do{nullptr};
};

///////////////////////////////////////////////////////////////////

struct queue {
    void push([[maybe_unused]] const std::any&) { log("push push push!"); }
};

void push_to(queue& q, const std::any& evt) { q.push(evt); }

struct communication {
    communication(queue_ref sink) noexcept : m_sink(sink) {}

    void do_something() {
        log("communication!");
        m_sink.push(42);
    }

private:
    queue_ref m_sink;
};

struct motorization {
    motorization(queue_ref sink) noexcept : m_sink(sink) {}

    void do_something() {
        log("motorization!");
        m_sink.push(17.f);
    }

private:
    queue_ref m_sink;
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
    communication m_com{queue_ref(tag{}, m_queue)};
    motorization m_mot{queue_ref(tag{}, m_queue)};
};

int main() {
    executer ex{};
    ex.do_something();
}

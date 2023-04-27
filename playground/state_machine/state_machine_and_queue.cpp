

// ##########################################################
//  UTILITIES
//  #########################################################

#include <cstdio>
#include <type_traits>

namespace utils {

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

}  // namespace utils

namespace meta {

template <typename...>
struct sequence {};

template <typename>
struct is_sequence : std::false_type {};

template <typename T>
inline constexpr bool is_sequence_v = is_sequence<T>::value;

template <typename... Ts>
struct is_sequence<sequence<Ts...>> : std::true_type {};

namespace detail {
template <typename, typename>
struct sequence_contains {
    static constexpr bool value = false;
};

template <typename T>
struct sequence_contains<T, sequence<>> {
    static constexpr bool value = false;
};

template <typename T, typename... Ts>
struct sequence_contains<T, sequence<T, Ts...>> {
    static constexpr bool value = true;
};

template <typename T, typename U, typename... Ts>
struct sequence_contains<T, sequence<U, Ts...>> {
    static constexpr bool value = sequence_contains<T, sequence<Ts...>>::value;
};

template <typename, typename>
struct sequence_append;

template <typename T, typename... Ts>
struct sequence_append<T, sequence<Ts...>> {
    using type = sequence<Ts..., T>;
};

template <typename... SEQ1_Ts, typename... SEQ2_Ts>
struct sequence_append<sequence<SEQ1_Ts...>, sequence<SEQ2_Ts...>> {
    using type = sequence<SEQ1_Ts..., SEQ2_Ts...>;
};
}  // namespace detail

template <typename, typename>
struct contains : std::false_type {};

template <typename T, typename U>
inline constexpr bool contains_v = contains<T, U>::value;

template <typename, typename>
struct append;

template <typename T, typename U>
using append_t = typename append<T, U>::type;

template <typename T, typename... Ts>
struct append<T, sequence<Ts...>> {
    using type = typename detail::sequence_append<T, sequence<Ts...>>::type;
};

template <typename U, typename... Ts>
struct contains<U, sequence<Ts...>> : detail::sequence_contains<U, sequence<Ts...>> {};

namespace concepts {
template <typename T, typename U>
concept contains = ::meta::contains_v<T, U>;

template <typename T>
concept sequence = ::meta::is_sequence_v<T>;
}  // namespace concepts
};  // namespace meta

// ##########################################################
//  STATES
//  #########################################################

namespace meta {
template <typename>
struct incoming_events {
    using type = ::meta::sequence<>;
};

template <typename T>
using incoming_events_t = typename incoming_events<T>::type;
}  // namespace meta

namespace states {
struct component_evt_1 {};
struct component_evt_2 {};

struct component;

struct state_evt_1 {};
struct state_evt_2 {};
struct state_evt_3 {};

struct state_machine;
}  // namespace states

namespace meta {
template <>
struct incoming_events<::states::component> {
    using type = ::meta::sequence<::states::component_evt_1, ::states::component_evt_2>;
};
}  // namespace meta

namespace states {

struct component {
    void on_event([[maybe_unused]] const component_evt_1& evt) noexcept { utils::log("component: event1"); }

    void on_event([[maybe_unused]] const component_evt_2& evt) noexcept { utils::log("component: event2"); }
};

inline void on_event(component& comp, const auto& evt) noexcept {
    static_assert(::meta::contains_v<std::decay_t<decltype(evt)>, meta::incoming_events_t<component>>);
    comp.on_event(evt);
}

}  // namespace states

namespace meta {
template <>
struct incoming_events<::states::state_machine> {
    using type = ::meta::sequence<::states::state_evt_1, ::states::state_evt_2, ::states::state_evt_3>;
};
}  // namespace meta

#include <tuple>

namespace states {

namespace detail {
template <typename T>
struct state_read_operation {
    using type = void(void*, const std::decay_t<T>&);
};

template <typename... Ts>
struct state_read_operations {
    using type = std::tuple<typename state_read_operation<Ts>::type*...>;
};

template <typename MSG_T, typename STATE_T>
struct can_receive : ::meta::contains<MSG_T, ::meta::incoming_events_t<STATE_T>> {};

template <typename MSG_T, typename STATE_T>
inline constexpr bool can_receive_v = can_receive<MSG_T, STATE_T>::value;

template <typename>
struct state_ref;

template <typename... Ts>
struct state_ref<::meta::sequence<Ts...>> {
    using read_operations = typename state_read_operations<Ts...>::type;

    template <typename T>
    state_ref(T& obj)
        : m_ptr{std::addressof(obj)}
        , m_calls{[](void* ptr, const std::decay_t<Ts>& msg) {
            if constexpr (can_receive_v<std::decay_t<Ts>, T>) {
                incoming_events(*static_cast<T*>(ptr), msg);
            }
        }...} {}

    template <typename T>
    void incoming(const T& msg) const noexcept {
        std::get<typename detail::state_read_operation<T>::type*>(m_calls)(m_ptr, msg);
    }

private:
    template <typename T>
    friend void incoming_events(state_ref& ref, const T& msg) {
        ref.send(msg);
    }

    void* m_ptr{nullptr};
    read_operations m_calls{};
};

}  // namespace detail
}  // namespace states

namespace states {
struct state1 {
    void on_event([[maybe_unused]] const state_evt_1& evt) noexcept { utils::log("state1: event1"); }
};

void incoming_events(state1& s, const state_evt_1& evt) noexcept { s.on_event(evt); }

struct state2 {
    void on_event([[maybe_unused]] const state_evt_2& evt) noexcept { utils::log("state2: event2"); }
    void on_event([[maybe_unused]] const state_evt_3& evt) noexcept { utils::log("state2: event3"); }
};

void incoming_events(state2& s, const auto& evt) noexcept { s.on_event(evt); }

struct state3 {
    void on_event([[maybe_unused]] const state_evt_3& evt) noexcept { utils::log("state3: event3"); }
};

void incoming_events(state3& s, const state_evt_3& evt) noexcept { s.on_event(evt); }
}  // namespace states

namespace meta {

template <>
struct incoming_events<::states::state1> {
    using type = ::meta::sequence<::states::state_evt_1>;
};
template <>
struct incoming_events<::states::state2> {
    using type = ::meta::sequence<::states::state_evt_2, ::states::state_evt_3>;
};
template <>
struct incoming_events<::states::state3> {
    using type = ::meta::sequence<::states::state_evt_3>;
};
}  // namespace meta

namespace states {

struct state_machine {
    using ref_type = detail::state_ref<::meta::incoming_events_t<state_machine>>;

    void on_event([[maybe_unused]] const state_evt_1& evt) noexcept {
        utils::log("state_machine: event1");
        m_ref.incoming(evt);
        m_ref = ref_type{m_s2};
    }

    void on_event([[maybe_unused]] const state_evt_2& evt) noexcept {
        utils::log("state_machine: event2");
        m_ref.incoming(evt);
        m_ref = ref_type{m_s3};
    }

    void on_event([[maybe_unused]] const state_evt_3& evt) noexcept {
        utils::log("state_machine: event3");
        m_ref.incoming(evt);
        m_ref = ref_type{m_s1};
    }

    state1 m_s1{};
    state2 m_s2{};
    state3 m_s3{};
    ref_type m_ref{m_s1};
};

inline void on_event(state_machine& sm, const auto& evt) noexcept {
    static_assert(::meta::contains_v<std::decay_t<decltype(evt)>, meta::incoming_events_t<state_machine>>);
    sm.on_event(evt);
}

}  // namespace states

// ##########################################################
//  QUEUE
//  #########################################################

#include <concepts>
#include <deque>
#include <any>

namespace detail {
class any_queue {
public:
    using element_type = std::any;
    using queue_type   = std::deque<element_type>;
    using size_type    = typename queue_type::size_type;

    inline bool empty() const noexcept { return m_queue.empty(); }
    inline auto size() const noexcept -> size_type { return m_queue.size(); }

    inline element_type pop_top() noexcept {
        const element_type elm = m_queue.front();
        m_queue.pop_front();
        return elm;
    }

    inline void push(const element_type& elm) { m_queue.emplace_back(elm); }

    inline void push(element_type&& elm) { m_queue.emplace_back(std::move(elm)); }

private:
    queue_type m_queue;
};

template <typename>
struct any_dispatcher;

template <typename... Ts>
struct any_dispatcher<::meta::sequence<Ts...>> {
    template <typename T>
    static void dispatch_impl(const std::any& obj, auto& receiver) noexcept {
        if (obj.type() != typeid(T)) {
            return;
        }

        const T& evt = std::any_cast<const T&>(obj);
        receiver.template dispatch<T>(evt);
    }

    static void dispatch(const std::any& obj, auto& receiver) noexcept {
        ((void)dispatch_impl<Ts>(obj, receiver), ...);
    }
};

template <::meta::concepts::sequence INCOMING_MSGs>
class evt_queue {
public:
    using message_types = INCOMING_MSGs;

    using queue_type      = any_queue;
    using element_type    = queue_type::element_type;
    using size_type       = queue_type::size_type;
    using dispatcher_type = any_dispatcher<message_types>;

    inline bool empty() const noexcept { return m_queue.empty(); }
    inline auto size() const noexcept -> size_type { return m_queue.size(); }

    void dispatch_top(auto& receiver) noexcept {
        const auto elm = m_queue.pop_top();
        if (!elm.has_value()) {
            return;
        }
        dispatcher_type::dispatch(elm, receiver);
    }

    template <typename MSG_T>
        requires ::meta::concepts::contains<MSG_T, message_types>
    void push(const MSG_T& msg) {
        m_queue.push(msg);
    }

private:
    any_queue m_queue;
};
}  // namespace detail

#include <tuple>
#include <functional>

namespace {

namespace detail {
template <typename T>
struct events_help;

template <>
struct events_help<::meta::sequence<>> {
    using type = ::meta::sequence<>;
};

template <typename T>
struct events_help<::meta::sequence<T>> {
    using type = ::meta::incoming_events_t<T>;
};

template <typename T, typename... Ts>
struct events_help<::meta::sequence<T, Ts...>> {
    using type = ::meta::append_t<::meta::incoming_events_t<T>, typename events_help<::meta::sequence<Ts...>>::type>;
};

template <typename... Ts>
struct events {
    using type = typename events_help<::meta::sequence<Ts...>>::type;
};

template <typename... Ts>
using events_t = typename events<Ts...>::type;
}  // namespace detail

template <typename... COMPONENT_Ts>
class event_queue {
public:
    using message_types       = detail::events_t<COMPONENT_Ts...>;
    using component_list_type = std::tuple<std::reference_wrapper<COMPONENT_Ts>...>;

    using queue_type   = ::detail::evt_queue<message_types>;
    using element_type = typename queue_type::element_type;
    using size_type    = typename queue_type::size_type;

    inline bool empty() const noexcept { return m_queue.empty(); }
    inline auto size() const noexcept -> size_type { return m_queue.size(); }

    template <typename... Ts>
    event_queue(Ts&... objs) : m_components(std::ref(objs)...){};

    template <typename MSG_T>
        requires ::meta::concepts::contains<MSG_T, message_types>
    void dispatch(const MSG_T& msg) {
        std::apply([this, &msg](auto&... objs) { (dispatch(msg, objs), ...); }, m_components);
    }

    void dispatch_top() noexcept { m_queue.dispatch_top(*this); }

    template <typename MSG_T>
        requires ::meta::concepts::contains<MSG_T, message_types>
    void push(const MSG_T& msg) {
        m_queue.push(msg);
    }

private:
    template <typename EVT_T, typename COMP_T>
    void dispatch(const EVT_T& evt, std::reference_wrapper<COMP_T>& obj) {
        if constexpr (::meta::contains_v<EVT_T, ::meta::incoming_events_t<COMP_T>>) {
            ::states::on_event(obj, evt);
            return;
        }
    }

    queue_type m_queue{};
    component_list_type m_components;
};

}  // namespace

int main() {
    states::component c1{};
    states::state_machine sm{};

    using queue_type = event_queue<states::component, states::state_machine>;
    queue_type q{c1, sm};

    q.push(states::component_evt_1{});
    q.push(states::component_evt_2{});
    q.push(states::state_evt_1{});
    q.push(states::state_evt_3{});
    q.push(states::state_evt_1{});
    q.push(states::state_evt_2{});
    q.push(states::state_evt_3{});

    while (!q.empty()) {
        q.dispatch_top();
    }
}

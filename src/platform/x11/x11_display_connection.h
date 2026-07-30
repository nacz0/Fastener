#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace fst::detail {

class X11DisplayConnection {
public:
    using CloseFunction = std::function<void(void*)>;

    X11DisplayConnection() = default;

    static X11DisplayConnection adopt(
        void* display,
        CloseFunction close) {
        X11DisplayConnection connection;
        if (display) {
            connection.m_state =
                std::make_shared<State>(display, std::move(close));
        }
        return connection;
    }

    void* get() const {
        return m_state ? m_state->display : nullptr;
    }

    void reset() {
        m_state.reset();
    }

private:
    struct State {
        State(void* displayValue, CloseFunction closeValue)
            : display(displayValue),
              close(std::move(closeValue)) {}

        ~State() {
            if (display && close) {
                close(display);
            }
        }

        void* display;
        CloseFunction close;
    };

    std::shared_ptr<State> m_state;
};

} // namespace fst::detail

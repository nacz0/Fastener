#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace fst::detail {

class WidgetStateRegistry {
public:
    template<typename T>
    T& get() {
        const std::size_t slot = stateSlot<T>();
        if (slot >= states.size()) {
            states.resize(slot + 1);
        }
        if (!states[slot]) {
            states[slot] = std::make_unique<StateHolder<T>>();
        }
        return static_cast<StateHolder<T>&>(*states[slot]).value;
    }

private:
    struct StateHolderBase {
        virtual ~StateHolderBase() = default;
    };

    template<typename T>
    struct StateHolder final : StateHolderBase {
        T value;
    };

    static std::size_t allocateSlot() {
        static std::atomic_size_t nextSlot{0};
        return nextSlot.fetch_add(1, std::memory_order_relaxed);
    }

    template<typename T>
    static std::size_t stateSlot() {
        static const std::size_t slot = allocateSlot();
        return slot;
    }

    std::vector<std::unique_ptr<StateHolderBase>> states;
};

} // namespace fst::detail

#pragma once

#include "fastener/core/context.h"

namespace fst {

/**
 * @brief RAII scope for automatic context stack management.
 * 
 * WidgetScope pushes a Context onto the thread-local context stack on construction
 * and pops it on destruction. This enables safe multi-context scenarios and 
 * automatic cleanup.
 * 
 * Usage:
 * @code
 *   void MyWidget(Context& ctx) {
 *       WidgetScope scope(ctx);
 *       auto wc = getWidgetContext();  // Uses ctx from stack
 *       // ... widget code ...
 *   }  // Automatically pops ctx from stack
 * @endcode
 */
class WidgetScope {
public:
    /**
     * @brief Construct scope and push context onto the stack.
     * @param ctx Context to push onto the thread-local stack.
     */
    explicit WidgetScope(Context& ctx) : m_ctx(&ctx) {
        Context::pushContext(m_ctx);
    }
    
    /**
     * @brief Destructor pops the context from the stack.
     */
    ~WidgetScope() {
        Context::popContext();
    }
    
    // Non-copyable, non-movable
    WidgetScope(const WidgetScope&) = delete;
    WidgetScope& operator=(const WidgetScope&) = delete;
    WidgetScope(WidgetScope&&) = delete;
    WidgetScope& operator=(WidgetScope&&) = delete;
    
    /**
     * @brief Get the context managed by this scope.
     */
    Context& context() const { return *m_ctx; }

private:
    Context* m_ctx;
};

} // namespace fst

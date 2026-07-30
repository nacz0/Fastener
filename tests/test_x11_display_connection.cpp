#include <gtest/gtest.h>
#include "../src/platform/x11/x11_display_connection.h"

using fst::detail::X11DisplayConnection;

TEST(X11DisplayConnectionTest, SharedWindowsCloseDisplayAfterLastOwner) {
    int fakeDisplay = 0;
    int closeCalls = 0;
    void* closedHandle = nullptr;

    auto mainConnection = X11DisplayConnection::adopt(
        &fakeDisplay,
        [&](void* handle) {
            ++closeCalls;
            closedHandle = handle;
        });

    {
        auto childConnection = mainConnection;
        mainConnection.reset();
        EXPECT_EQ(closeCalls, 0);
        EXPECT_EQ(childConnection.get(), &fakeDisplay);
    }

    EXPECT_EQ(closeCalls, 1);
    EXPECT_EQ(closedHandle, &fakeDisplay);
}

TEST(X11DisplayConnectionTest, EmptyConnectionNeverInvokesCloser) {
    int closeCalls = 0;

    auto connection = X11DisplayConnection::adopt(
        nullptr,
        [&](void*) { ++closeCalls; });
    connection.reset();

    EXPECT_EQ(closeCalls, 0);
}

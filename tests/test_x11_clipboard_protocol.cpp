#include <gtest/gtest.h>
#include "../src/platform/x11/x11_clipboard_protocol.h"

using namespace fst::detail;

namespace {

constexpr X11ClipboardAtoms kAtoms{
    1, // targets
    2, // utf8String
    3, // string
    4  // atom
};

} // namespace

TEST(X11ClipboardProtocolTest, TargetsRequestAdvertisesEverySupportedEncoding) {
    const auto response =
        buildX11ClipboardResponse(kAtoms.targets, kAtoms, "hello");

    ASSERT_TRUE(response.supported());
    EXPECT_EQ(response.propertyType, kAtoms.atom);
    EXPECT_EQ(response.format, 32);
    EXPECT_EQ(
        response.atoms,
        (std::vector<X11Atom>{kAtoms.targets, kAtoms.utf8String, kAtoms.string}));
    EXPECT_TRUE(response.bytes.empty());
}

TEST(X11ClipboardProtocolTest, Utf8RequestPreservesEmbeddedNullBytes) {
    const std::string text{"a\0b", 3};

    const auto response =
        buildX11ClipboardResponse(kAtoms.utf8String, kAtoms, text);

    ASSERT_TRUE(response.supported());
    EXPECT_EQ(response.propertyType, kAtoms.utf8String);
    EXPECT_EQ(response.format, 8);
    EXPECT_EQ(
        response.bytes,
        (std::vector<unsigned char>{'a', '\0', 'b'}));
    EXPECT_TRUE(response.atoms.empty());
}

TEST(X11ClipboardProtocolTest, LegacyStringRequestUsesItsRequestedType) {
    const auto response =
        buildX11ClipboardResponse(kAtoms.string, kAtoms, "plain");

    ASSERT_TRUE(response.supported());
    EXPECT_EQ(response.propertyType, kAtoms.string);
    EXPECT_EQ(response.format, 8);
}

TEST(X11ClipboardProtocolTest, UnsupportedTargetProducesNoPropertyData) {
    const auto response = buildX11ClipboardResponse(99, kAtoms, "hello");

    EXPECT_FALSE(response.supported());
    EXPECT_EQ(response.propertyType, X11Atom{0});
    EXPECT_EQ(response.format, 0);
    EXPECT_TRUE(response.bytes.empty());
    EXPECT_TRUE(response.atoms.empty());
}

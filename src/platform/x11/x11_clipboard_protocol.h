#pragma once

#include <string>
#include <vector>

namespace fst::detail {

using X11Atom = unsigned long;

struct X11ClipboardAtoms {
    X11Atom targets = 0;
    X11Atom utf8String = 0;
    X11Atom string = 0;
    X11Atom atom = 0;
};

struct X11ClipboardResponse {
    X11Atom propertyType = 0;
    int format = 0;
    std::vector<unsigned char> bytes;
    std::vector<X11Atom> atoms;

    bool supported() const {
        return propertyType != 0;
    }
};

inline X11ClipboardResponse buildX11ClipboardResponse(
    X11Atom requestedTarget,
    const X11ClipboardAtoms& knownAtoms,
    const std::string& ownedText) {
    X11ClipboardResponse response;

    if (requestedTarget == knownAtoms.targets) {
        response.propertyType = knownAtoms.atom;
        response.format = 32;
        response.atoms = {
            knownAtoms.targets,
            knownAtoms.utf8String,
            knownAtoms.string};
    } else if (requestedTarget == knownAtoms.utf8String ||
               requestedTarget == knownAtoms.string) {
        response.propertyType = requestedTarget;
        response.format = 8;
        response.bytes.assign(ownedText.begin(), ownedText.end());
    }

    return response;
}

} // namespace fst::detail

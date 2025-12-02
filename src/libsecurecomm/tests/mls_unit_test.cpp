#include "securecomm/mls_manager.hpp"
#include "securecomm/envelope.hpp"
#include <iostream>
#include <cassert>
#include <sodium.h>

using namespace securecomm;

int main() {
    if (sodium_init() < 0) return 1;

    MLSManager m;

    auto gid = m.create_group("team-alpha");

    m.add_member(gid, "alice");
    m.add_member(gid, "bob");
    m.add_member(gid, "carol");

    std::string msg = "group hello";
    std::vector<uint8_t> pt(msg.begin(), msg.end());

    Envelope e = m.encrypt_group_message(gid, "alice", pt);

    auto bob_plain = m.decrypt_group_message(gid, "bob", e);
    assert(bob_plain.has_value());
    assert(bob_plain.value() == pt);

    m.remove_member(gid, "carol");

    std::string m2 = "after remove";
    std::vector<uint8_t> pt2(m2.begin(), m2.end());
    Envelope e2 = m.encrypt_group_message(gid, "bob", pt2);

    auto alice_plain = m.decrypt_group_message(gid, "alice", e2);
    assert(alice_plain.has_value());
    assert(alice_plain.value() == pt2);

    std::cout << "MLS unit test: OK\n";
    return 0;
}

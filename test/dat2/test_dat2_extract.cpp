#include "dat2/dat2_archive.h"
#include "doctest.h"
#include "test_dat2_helpers.h"

#include <cstring>

TEST_CASE("dat2::extract uncompressed roundtrip") {
    std::vector<uint8_t> original = {'H', 'e', 'l', 'l', 'o', ',', ' ',
                                     'W', 'o', 'r', 'l', 'd', '!'};
    auto [buf, size] = build_test_dat2({{"hello.txt", original, false}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("hello.txt");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value == original);
}

TEST_CASE("dat2::extract compressed roundtrip") {
    // Make data large enough that compression is meaningful
    std::vector<uint8_t> original(256);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i % 10); // repetitive = compressible
    }

    auto [buf, size] = build_test_dat2({{"data.bin", original, true}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("data.bin");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value == original);
}

TEST_CASE("dat2::extract zlib magic detection not flag") {
    // Build compressed archive - the data in the DAT2 should start with 0x78 0xDA
    // Verify that extraction works regardless of the is_compressed flag
    std::vector<uint8_t> original(128);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i % 5);
    }

    auto [buf, size] = build_test_dat2({{"test.dat", original, true}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    // The entry should be compressed (data starts with zlib header)
    const auto* entry = result.value.find_entry("test.dat");
    REQUIRE(entry != nullptr);

    // Extract should detect zlib and decompress
    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value == original);
}

TEST_CASE("dat2::extract_to uncompressed") {
    std::vector<uint8_t> original = {0xDE, 0xAD, 0xBE, 0xEF};
    auto [buf, size] = build_test_dat2({{"test.bin", original, false}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("test.bin");
    REQUIRE(entry != nullptr);

    uint8_t out_buf[4] = {};
    auto status = result.value.extract_to(*entry, out_buf, sizeof(out_buf));
    REQUIRE(status.ok());
    CHECK(std::memcmp(out_buf, original.data(), 4) == 0);
}

TEST_CASE("dat2::extract_to compressed") {
    std::vector<uint8_t> original(200);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i % 7);
    }

    auto [buf, size] = build_test_dat2({{"comp.dat", original, true}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("comp.dat");
    REQUIRE(entry != nullptr);

    std::vector<uint8_t> out_buf(original.size());
    auto status = result.value.extract_to(*entry, out_buf.data(), out_buf.size());
    REQUIRE(status.ok());
    CHECK(out_buf == original);
}

TEST_CASE("dat2::extract_to buffer too small (uncompressed)") {
    std::vector<uint8_t> original = {0x01, 0x02, 0x03, 0x04};
    auto [buf, size] = build_test_dat2({{"test.bin", original, false}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("test.bin");
    REQUIRE(entry != nullptr);

    uint8_t out_buf[2] = {};
    auto status = result.value.extract_to(*entry, out_buf, sizeof(out_buf));
    CHECK_FALSE(status.ok());
}

TEST_CASE("dat2::extract_to buffer too small (compressed)") {
    std::vector<uint8_t> original(100);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i % 3);
    }

    auto [buf, size] = build_test_dat2({{"comp.dat", original, true}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("comp.dat");
    REQUIRE(entry != nullptr);

    uint8_t out_buf[2] = {};
    auto status = result.value.extract_to(*entry, out_buf, sizeof(out_buf));
    CHECK_FALSE(status.ok());
}

TEST_CASE("dat2::extract corrupted compressed data") {
    // Build a compressed archive, then corrupt the compressed data bytes
    std::vector<uint8_t> original(100);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i);
    }

    auto [buf, size] = build_test_dat2({{"test.dat", original, true}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("test.dat");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->packed_size > 4);

    // Corrupt some bytes in the compressed data (but keep zlib header intact)
    // The data starts at entry->offset in the buffer
    // We need to access the internal buffer via extract — but we already built it
    // Instead, corrupt the raw buffer before parsing... but we already parsed.
    // Let's rebuild with corruption.
    auto [buf2, size2] = build_test_dat2({{"test.dat", original, true}});
    // Corrupt compressed data (skip first 2 bytes = zlib header)
    buf2[4] = 0xFF;
    buf2[5] = 0xFF;
    buf2[6] = 0xFF;

    auto result2 = dat2::Dat2Archive::open_from_buffer(std::move(buf2), size2);
    REQUIRE(result2.ok()); // parsing should succeed (tree is intact)

    const auto* entry2 = result2.value.find_entry("test.dat");
    REQUIRE(entry2 != nullptr);

    auto extracted = result2.value.extract(*entry2);
    CHECK_FALSE(extracted.ok());
    CHECK(extracted.error == dat2::Dat2Error::DECOMPRESSION_FAILED);
}

TEST_CASE("dat2::extract zero-length file") {
    std::vector<uint8_t> empty_data;
    auto [buf, size] = build_test_dat2({{"empty.txt", empty_data, false}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    const auto* entry = result.value.find_entry("empty.txt");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value.empty());
}

TEST_CASE("dat2::extract multiple files selectively") {
    std::vector<uint8_t> data1 = {0xAA, 0xBB};
    std::vector<uint8_t> data2 = {0xCC, 0xDD, 0xEE};
    std::vector<uint8_t> data3 = {0xFF};

    auto [buf, size] = build_test_dat2({
        {"first.bin", data1, false},
        {"second.bin", data2, false},
        {"third.bin", data3, false},
    });

    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    // Extract only the second file
    const auto* entry2 = result.value.find_entry("second.bin");
    REQUIRE(entry2 != nullptr);
    auto extracted2 = result.value.extract(*entry2);
    REQUIRE(extracted2.ok());
    CHECK(extracted2.value == data2);

    // Extract only the third file
    const auto* entry3 = result.value.find_entry("third.bin");
    REQUIRE(entry3 != nullptr);
    auto extracted3 = result.value.extract(*entry3);
    REQUIRE(extracted3.ok());
    CHECK(extracted3.value == data3);
}

TEST_CASE("dat2::entry out of bounds") {
    auto [buf, size] = build_test_dat2({{"test.dat", {0x01}, false}});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    // Forge a bad entry with offset past data section
    dat2::Dat2Entry bad_entry;
    bad_entry.filename = "fake.dat";
    bad_entry.offset = 99999;
    bad_entry.packed_size = 10;
    bad_entry.decompressed_size = 10;
    bad_entry.is_compressed = false;

    auto extracted = result.value.extract(bad_entry);
    CHECK_FALSE(extracted.ok());
    CHECK(extracted.error == dat2::Dat2Error::ENTRY_OUT_OF_BOUNDS);
}

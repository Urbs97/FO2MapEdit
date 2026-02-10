#include "dat2/dat2_archive.h"
#include "doctest.h"
#include "test_dat2_helpers.h"

#include <cstring>

TEST_CASE("dat2::parse empty archive (0 files)") {
    auto [buf, size] = build_test_dat2({});
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 0);
    CHECK(result.value.entries().empty());
}

TEST_CASE("dat2::parse single uncompressed file") {
    std::vector<uint8_t> data = { 'H', 'e', 'l', 'l', 'o' };
    auto [buf, size] = build_test_dat2({
        { "art\\tiles\\test.frm", data, false }
    });
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 1);

    const auto& entries = result.value.entries();
    CHECK(entries[0].filename == "art\\tiles\\test.frm");
    CHECK(entries[0].offset == 0);
    CHECK(entries[0].packed_size == 5);
    CHECK(entries[0].decompressed_size == 5);
    CHECK(entries[0].is_compressed == false);
}

TEST_CASE("dat2::parse multiple files") {
    std::vector<uint8_t> data1 = { 0x01, 0x02, 0x03 };
    std::vector<uint8_t> data2 = { 0x04, 0x05, 0x06, 0x07 };
    std::vector<uint8_t> data3 = { 0x08 };

    auto [buf, size] = build_test_dat2({
        { "file1.dat", data1, false },
        { "dir\\file2.dat", data2, false },
        { "a\\b\\c.txt", data3, false },
    });

    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 3);

    const auto& entries = result.value.entries();
    CHECK(entries[0].filename == "file1.dat");
    CHECK(entries[0].packed_size == 3);
    CHECK(entries[1].filename == "dir\\file2.dat");
    CHECK(entries[1].packed_size == 4);
    CHECK(entries[2].filename == "a\\b\\c.txt");
    CHECK(entries[2].packed_size == 1);
}

TEST_CASE("dat2::parse file too small") {
    // Less than 12 bytes
    auto buf = std::make_unique<uint8_t[]>(8);
    std::memset(buf.get(), 0, 8);
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), 8);
    CHECK_FALSE(result.ok());
    CHECK(result.error == dat2::Dat2Error::FILE_TOO_SMALL);
}

TEST_CASE("dat2::parse file_size mismatch") {
    // Build a valid archive then corrupt the file_size field
    auto [buf, size] = build_test_dat2({});
    // file_size is the last 4 bytes — write a wrong value
    buf[size - 4] = 0xFF;
    buf[size - 3] = 0xFF;
    buf[size - 2] = 0x00;
    buf[size - 1] = 0x00;
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    CHECK_FALSE(result.ok());
    CHECK(result.error == dat2::Dat2Error::FILE_SIZE_MISMATCH);
}

TEST_CASE("dat2::parse invalid tree_size") {
    // Build an empty archive then corrupt tree_size to be impossibly large
    auto [buf, size] = build_test_dat2({});
    // tree_size is at offset size-8..size-5
    // Set it to something larger than the file
    buf[size - 8] = 0xFF;
    buf[size - 7] = 0xFF;
    buf[size - 6] = 0x00;
    buf[size - 5] = 0x00;
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    CHECK_FALSE(result.ok());
    CHECK(result.error == dat2::Dat2Error::TREE_SIZE_INVALID);
}

TEST_CASE("dat2::parse truncated tree entry") {
    // Build a valid 1-file archive, then reduce num_files field
    // to claim 2 files when only 1 tree entry exists
    auto [buf, size] = build_test_dat2({
        { "test.dat", { 0x01 }, false }
    });

    // Find num_files field and set it to 2
    // num_files is right before tree entries, which is right before tree_size + file_size
    // We know the layout, so let's overwrite num_files
    // In a 1-file archive: data(1) + num_files(4) + tree_entry + tree_size(4) + file_size(4)
    // num_files is at offset 1 (data is 1 byte)
    buf[1] = 2; // claim 2 files
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    CHECK_FALSE(result.ok());
    CHECK(result.error == dat2::Dat2Error::TREE_PARSE_ERROR);
}

TEST_CASE("dat2::find_entry case insensitive") {
    auto [buf, size] = build_test_dat2({
        { "Art\\Tiles\\Test.FRM", { 0x01 }, false }
    });
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    // Exact match
    CHECK(result.value.find_entry("Art\\Tiles\\Test.FRM") != nullptr);
    // Different case
    CHECK(result.value.find_entry("art\\tiles\\test.frm") != nullptr);
    // Forward slashes
    CHECK(result.value.find_entry("art/tiles/test.frm") != nullptr);
    // Mixed
    CHECK(result.value.find_entry("ART/TILES/TEST.FRM") != nullptr);
    // Non-existent
    CHECK(result.value.find_entry("art\\tiles\\other.frm") == nullptr);
}

TEST_CASE("dat2::find_entry nullptr path") {
    auto [buf, size] = build_test_dat2({
        { "test.dat", { 0x01 }, false }
    });
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());
    // Empty string should not match
    CHECK(result.value.find_entry("") == nullptr);
}

TEST_CASE("dat2::error string coverage") {
    CHECK(std::strlen(dat2::dat2_error_str(dat2::Dat2Error::OK)) > 0);
    CHECK(std::strlen(dat2::dat2_error_str(dat2::Dat2Error::FILE_OPEN_FAILED)) > 0);
    CHECK(std::strlen(dat2::dat2_error_str(dat2::Dat2Error::DECOMPRESSION_FAILED)) > 0);
}

TEST_CASE("dat2::Dat2Archive move semantics") {
    auto [buf, size] = build_test_dat2({
        { "test.dat", { 0x01, 0x02 }, false }
    });
    auto result = dat2::Dat2Archive::open_from_buffer(std::move(buf), size);
    REQUIRE(result.ok());

    // Move construct
    dat2::Dat2Archive moved(std::move(result.value));
    CHECK(moved.file_count() == 1);

    // Move assign
    dat2::Dat2Archive assigned;
    assigned = std::move(moved);
    CHECK(assigned.file_count() == 1);
    CHECK(assigned.entries()[0].filename == "test.dat");
}

#include "dat2/dat2_archive.h"
#include "dat2/dat2_writer.h"
#include "doctest.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Helper: write archive to a temp file, read it back into a buffer, return Dat2Archive.
static dat2::Dat2Result<dat2::Dat2Archive>
write_and_reopen(const std::vector<dat2::Dat2WriteEntry>& entries,
                 const dat2::Dat2WriteOptions& opts) {
    namespace fs = std::filesystem;
    fs::path tmp = fs::temp_directory_path() / "test_dat2_writer.dat";
    std::string path = tmp.u8string();

    auto status = dat2::write_archive(path.c_str(), entries, opts);
    if (!status.ok()) {
        return dat2::Dat2Result<dat2::Dat2Archive>::fail(status.error);
    }

    auto result = dat2::Dat2Archive::open(path.c_str());
    fs::remove(tmp);
    return result;
}

TEST_CASE("dat2::write_archive single uncompressed file roundtrip") {
    std::vector<uint8_t> original = {'H', 'e', 'l', 'l', 'o'};
    std::vector<dat2::Dat2WriteEntry> entries = {{"test\\hello.txt", original}};

    auto result = write_and_reopen(entries, {false});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 1);

    const auto* entry = result.value.find_entry("test\\hello.txt");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value == original);
}

TEST_CASE("dat2::write_archive single compressed file roundtrip") {
    std::vector<uint8_t> original(256);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<uint8_t>(i % 10);
    }

    std::vector<dat2::Dat2WriteEntry> entries = {{"data\\numbers.bin", original}};

    auto result = write_and_reopen(entries, {true});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 1);

    const auto* entry = result.value.find_entry("data\\numbers.bin");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value == original);
}

TEST_CASE("dat2::write_archive multiple files roundtrip") {
    std::vector<uint8_t> data1 = {0xAA, 0xBB, 0xCC};
    std::vector<uint8_t> data2 = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<uint8_t> data3 = {0xFF};

    std::vector<dat2::Dat2WriteEntry> entries = {
        {"dir\\file1.bin", data1},
        {"dir\\file2.bin", data2},
        {"file3.bin", data3},
    };

    auto result = write_and_reopen(entries, {false});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 3);

    auto e1 = result.value.extract(*result.value.find_entry("dir\\file1.bin"));
    auto e2 = result.value.extract(*result.value.find_entry("dir\\file2.bin"));
    auto e3 = result.value.extract(*result.value.find_entry("file3.bin"));
    REQUIRE(e1.ok());
    REQUIRE(e2.ok());
    REQUIRE(e3.ok());
    CHECK(e1.value == data1);
    CHECK(e2.value == data2);
    CHECK(e3.value == data3);
}

TEST_CASE("dat2::write_archive empty file roundtrip") {
    std::vector<dat2::Dat2WriteEntry> entries = {{"empty.txt", {}}};

    auto result = write_and_reopen(entries, {false});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 1);

    const auto* entry = result.value.find_entry("empty.txt");
    REQUIRE(entry != nullptr);

    auto extracted = result.value.extract(*entry);
    REQUIRE(extracted.ok());
    CHECK(extracted.value.empty());
}

TEST_CASE("dat2::write_archive empty archive") {
    std::vector<dat2::Dat2WriteEntry> entries;

    auto result = write_and_reopen(entries, {false});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 0);
}

TEST_CASE("dat2::collect_from_directory and write roundtrip") {
    namespace fs = std::filesystem;

    // Create a temporary directory structure
    fs::path tmp_dir = fs::temp_directory_path() / "test_dat2_collect";
    fs::remove_all(tmp_dir);
    fs::create_directories(tmp_dir / "subdir");

    // Write test files
    {
        std::ofstream f(tmp_dir / "root.txt");
        f << "root file";
    }
    {
        std::ofstream f(tmp_dir / "subdir" / "nested.txt");
        f << "nested file";
    }

    auto collected = dat2::collect_from_directory(tmp_dir.c_str());
    REQUIRE(collected.ok());
    CHECK(collected.value.size() == 2);

    // Write and reopen
    auto result = write_and_reopen(collected.value, {true});
    REQUIRE(result.ok());
    CHECK(result.value.file_count() == 2);

    // Verify root.txt
    const auto* root_entry = result.value.find_entry("root.txt");
    REQUIRE(root_entry != nullptr);
    auto root_data = result.value.extract(*root_entry);
    REQUIRE(root_data.ok());
    CHECK(std::string(root_data.value.begin(), root_data.value.end()) == "root file");

    // Verify subdir\nested.txt (backslash path)
    const auto* nested_entry = result.value.find_entry("subdir\\nested.txt");
    REQUIRE(nested_entry != nullptr);
    auto nested_data = result.value.extract(*nested_entry);
    REQUIRE(nested_data.ok());
    CHECK(std::string(nested_data.value.begin(), nested_data.value.end()) == "nested file");

    fs::remove_all(tmp_dir);
}

TEST_CASE("dat2::collect_from_archive roundtrip (repack)") {
    // Build an archive with known data
    std::vector<uint8_t> data1 = {0x10, 0x20, 0x30};
    std::vector<uint8_t> data2(100);
    for (size_t i = 0; i < data2.size(); i++) {
        data2[i] = static_cast<uint8_t>(i % 13);
    }

    std::vector<dat2::Dat2WriteEntry> original_entries = {
        {"art\\file1.frm", data1},
        {"data\\file2.dat", data2},
    };

    // Write original archive
    namespace fs = std::filesystem;
    fs::path tmp1 = fs::temp_directory_path() / "test_dat2_repack_src.dat";
    fs::path tmp2 = fs::temp_directory_path() / "test_dat2_repack_dst.dat";

    auto ws1 = dat2::write_archive(tmp1.c_str(), original_entries, {true});
    REQUIRE(ws1.ok());

    // Open it
    auto opened = dat2::Dat2Archive::open(tmp1.c_str());
    REQUIRE(opened.ok());

    // Collect from archive
    auto collected = dat2::collect_from_archive(opened.value);
    REQUIRE(collected.ok());
    CHECK(collected.value.size() == 2);

    // Write repacked archive
    auto ws2 = dat2::write_archive(tmp2.c_str(), collected.value, {true});
    REQUIRE(ws2.ok());

    // Reopen repacked and verify
    auto reopened = dat2::Dat2Archive::open(tmp2.c_str());
    REQUIRE(reopened.ok());
    CHECK(reopened.value.file_count() == 2);

    auto e1 = reopened.value.extract(*reopened.value.find_entry("art\\file1.frm"));
    auto e2 = reopened.value.extract(*reopened.value.find_entry("data\\file2.dat"));
    REQUIRE(e1.ok());
    REQUIRE(e2.ok());
    CHECK(e1.value == data1);
    CHECK(e2.value == data2);

    fs::remove(tmp1);
    fs::remove(tmp2);
}

TEST_CASE("dat2::collect_from_directory fails on nonexistent path") {
    auto result = dat2::collect_from_directory("/nonexistent/path/that/does/not/exist");
    CHECK_FALSE(result.ok());
    CHECK(result.error == dat2::Dat2Error::DIRECTORY_READ_FAILED);
}

TEST_CASE("dat2::write_archive fails on invalid output path") {
    std::vector<dat2::Dat2WriteEntry> entries = {{"test.txt", {0x41}}};

    auto status = dat2::write_archive("/nonexistent/dir/output.dat", entries, {false});
    CHECK_FALSE(status.ok());
    CHECK(status.error == dat2::Dat2Error::FILE_WRITE_FAILED);
}

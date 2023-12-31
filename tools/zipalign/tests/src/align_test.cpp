#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ZipAlign.h"

#include <filesystem>
#include <stdio.h>
#include <string>

#include <android-base/file.h>

using namespace android;
using namespace base;

// This load the whole file to memory so be careful!
static bool sameContent(const std::string& path1, const std::string& path2) {
  std::string f1;
  if (!ReadFileToString(path1, &f1)) {
    printf("Unable to read '%s' content: %m\n", path1.c_str());
    return false;
  }

  std::string f2;
  if (!ReadFileToString(path2, &f2)) {
    printf("Unable to read '%s' content %m\n", path1.c_str());
    return false;
  }

  if (f1.size() != f2.size()) {
    printf("File '%s' and '%s' are not the same\n", path1.c_str(), path2.c_str());
    return false;
  }

  return f1.compare(f2) == 0;
}

static std::string GetTestPath(const std::string& filename) {
  static std::string test_data_dir = android::base::GetExecutableDirectory() + "/tests/data/";
  return test_data_dir + filename;
}

static std::string GetTempPath(const std::string& filename) {
  std::filesystem::path temp_path = std::filesystem::path(testing::TempDir());
  temp_path += filename;
  return temp_path.string();
}

TEST(Align, Unaligned) {
  const std::string src = GetTestPath("unaligned.zip");
  const std::string dst = GetTempPath("unaligned_out.zip");
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, false, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, true, false, pageSize);
  ASSERT_EQ(0, verified);
}

TEST(Align, DoubleAligment) {
  const std::string src = GetTestPath("unaligned.zip");
  const std::string tmp = GetTempPath("da_aligned.zip");
  const std::string dst = GetTempPath("da_d_aligner.zip");
  int pageSize = 4096;

  int processed = process(src.c_str(), tmp.c_str(), 4, true, false, false, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(tmp.c_str(), 4, true, false, pageSize);
  ASSERT_EQ(0, verified);

  // Align the result of the previous run. Essentially double aligning.
  processed = process(tmp.c_str(), dst.c_str(), 4, true, false, false, pageSize);
  ASSERT_EQ(0, processed);

  verified = verify(dst.c_str(), 4, true, false, pageSize);
  ASSERT_EQ(0, verified);

  // Nothing should have changed between tmp and dst.
  std::string tmp_content;
  ASSERT_EQ(true, ReadFileToString(tmp, &tmp_content));

  std::string dst_content;
  ASSERT_EQ(true, ReadFileToString(dst, &dst_content));

  ASSERT_EQ(tmp_content, dst_content);
}

// Align a zip featuring a hole at the beginning. The
// hole in the archive is a delete entry in the Central
// Directory.
TEST(Align, Holes) {
  const std::string src = GetTestPath("holes.zip");
  const std::string dst = GetTempPath("holes_out.zip");
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, false, true, pageSize);
  ASSERT_EQ(0, verified);
}

// Align a zip where LFH order and CD entries differ.
TEST(Align, DifferenteOrders) {
  const std::string src = GetTestPath("diffOrders.zip");
  const std::string dst = GetTempPath("diffOrders_out.zip");
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, false, true, pageSize);
  ASSERT_EQ(0, verified);
}

TEST(Align, DirectoryEntryDoNotRequireAlignment) {
  const std::string src = GetTestPath("archiveWithOneDirectoryEntry.zip");
  int pageSize = 4096;
  int verified = verify(src.c_str(), 4, false, true, pageSize);
  ASSERT_EQ(0, verified);
}

TEST(Align, DirectoryEntry) {
  const std::string src = GetTestPath("archiveWithOneDirectoryEntry.zip");
  const std::string dst = GetTempPath("archiveWithOneDirectoryEntry_out.zip");
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);
  ASSERT_EQ(true, sameContent(src, dst));

  int verified = verify(dst.c_str(), 4, false, true, pageSize);
  ASSERT_EQ(0, verified);
}

class UncompressedSharedLibsTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
      src = GetTestPath("apkWithUncompressedSharedLibs.zip");
      dst = GetTempPath("apkWithUncompressedSharedLibs_out.zip");
    }

    static std::string src;
    static std::string dst;
};

std::string UncompressedSharedLibsTest::src;
std::string UncompressedSharedLibsTest::dst;

TEST_F(UncompressedSharedLibsTest, Unaligned) {
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, false, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, true, true, pageSize);
  ASSERT_NE(0, verified); // .so's not page-aligned
}

TEST_F(UncompressedSharedLibsTest, AlignedPageSize4kB) {
  int pageSize = 4096;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, true, true, pageSize);
  ASSERT_EQ(0, verified);
}

TEST_F(UncompressedSharedLibsTest, AlignedPageSize16kB) {
  int pageSize = 16384;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, true, true, pageSize);
  ASSERT_EQ(0, verified);
}

TEST_F(UncompressedSharedLibsTest, AlignedPageSize64kB) {
  int pageSize = 65536;

  int processed = process(src.c_str(), dst.c_str(), 4, true, false, true, pageSize);
  ASSERT_EQ(0, processed);

  int verified = verify(dst.c_str(), 4, true, true, pageSize);
  ASSERT_EQ(0, verified);
}

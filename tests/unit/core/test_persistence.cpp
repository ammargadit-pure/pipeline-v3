#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

extern "C" {
#include "ota/core/persistence.h"
}

static const char *TEST_DIR = "/tmp/ota-test-persistence";

class PersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        /* Clean up and recreate test directory */
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s && mkdir -p %s", TEST_DIR, TEST_DIR);
        (void)system(cmd);
    }
    void TearDown() override {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_DIR);
        (void)system(cmd);
    }
};

TEST_F(PersistenceTest, AtomicWriteReadRoundtrip)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/roundtrip.bin", TEST_DIR);

    const uint8_t data[] = "Hello, persistence!";
    size_t data_len = strlen((const char *)data);

    ASSERT_EQ(ota_atomic_write(path, data, data_len), OTA_OK);

    uint8_t *out_data = nullptr;
    size_t out_len = 0;
    ASSERT_EQ(ota_read_file(path, &out_data, &out_len), OTA_OK);

    EXPECT_EQ(out_len, data_len);
    EXPECT_EQ(memcmp(out_data, data, data_len), 0);

    free(out_data);
}

TEST_F(PersistenceTest, AtomicWriteJsonReadStr)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/test.json", TEST_DIR);

    const char *json = R"({"key": "value", "num": 42})";
    ASSERT_EQ(ota_atomic_write_json(path, json), OTA_OK);

    char *content = ota_read_file_str(path);
    ASSERT_NE(content, nullptr);
    EXPECT_STREQ(content, json);

    free(content);
}

TEST_F(PersistenceTest, ReadNonexistentFile)
{
    uint8_t *data = nullptr;
    size_t len = 0;
    ota_error_t err = ota_read_file("/tmp/ota-test-persistence/nonexistent.txt", &data, &len);
    EXPECT_EQ(err, OTA_ERR_NOT_FOUND);
    EXPECT_EQ(data, nullptr);
}

TEST_F(PersistenceTest, FileExistsTrue)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/exists.txt", TEST_DIR);

    const uint8_t data[] = "x";
    ASSERT_EQ(ota_atomic_write(path, data, 1), OTA_OK);

    EXPECT_TRUE(ota_file_exists(path));
    EXPECT_FALSE(ota_file_exists("/tmp/ota-test-persistence/does-not-exist.txt"));
}

TEST_F(PersistenceTest, FileExistsFalse)
{
    EXPECT_FALSE(ota_file_exists("/tmp/ota-test-persistence/nope.bin"));
    EXPECT_FALSE(ota_file_exists(nullptr));
}

TEST_F(PersistenceTest, MkdirPCreatesNested)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/a/b/c/d", TEST_DIR);

    ASSERT_EQ(ota_mkdir_p(path), OTA_OK);

    struct stat st;
    EXPECT_EQ(stat(path, &st), 0);
    EXPECT_TRUE(S_ISDIR(st.st_mode));
}

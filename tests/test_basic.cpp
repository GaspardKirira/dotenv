#include <dotenv/dotenv.hpp>

#include <cassert>
#include <fstream>
#include <string>

namespace
{
  void write_file(const std::string &path, const std::string &content)
  {
    std::ofstream out(path, std::ios::binary);
    assert(out && "failed to open temp test file");
    out << content;
    out.flush();
    assert(out && "failed to write temp test file");
  }

  void test_parse_basic()
  {
    const std::string path = "dotenv_test_basic.env";

    write_file(path,
               R"(# comment line
FOO=bar
HELLO = world
EMPTY=
SPACED =   value with spaces
INLINE=ok # trailing comment
QUOTED1="hello world"
QUOTED2='raw # keep'
ESCAPED="a\nb\tc\\\""
INVALID KEY=should_be_ignored
)");

    const dotenv::env_map m = dotenv::load(path);

    assert(m.at("FOO") == "bar");
    assert(m.at("HELLO") == "world");
    assert(m.at("EMPTY") == "");
    assert(m.at("SPACED") == "value with spaces");
    assert(m.at("INLINE") == "ok");
    assert(m.at("QUOTED1") == "hello world");
    assert(m.at("QUOTED2") == "raw # keep");

    // ESCAPED: verify we got real escapes for double quotes.
    // expected: a\nb\tc\" -> "a<LF>b<TAB>c\""
    assert(m.at("ESCAPED").size() >= 5);
    assert(m.at("ESCAPED").find('\n') != std::string::npos);
    assert(m.at("ESCAPED").find('\t') != std::string::npos);
    assert(m.at("ESCAPED").find('"') != std::string::npos);

    // Invalid key should be ignored (space in key)
    assert(m.find("INVALID KEY") == m.end());
  }

  void test_load_into_overwrite()
  {
    const std::string path = "dotenv_test_merge.env";
    write_file(path, "A=1\nB=2\n");

    dotenv::env_map m;
    m["A"] = "old";

    dotenv::load_into(m, path, false);
    assert(m.at("A") == "old");
    assert(m.at("B") == "2");

    dotenv::load_into(m, path, true);
    assert(m.at("A") == "1");
    assert(m.at("B") == "2");
  }

  void test_apply_env()
  {
    // We do not rely on the environment being empty.
    // We set a unique key and verify it matches.
    dotenv::env_map m;
    m["DOTENV_TEST_KEY"] = "123";

    dotenv::apply(m, true);
    const std::string v = dotenv::getenv_str("DOTENV_TEST_KEY");
    assert(v == "123");
  }
} // namespace

int main()
{
  test_parse_basic();
  test_load_into_overwrite();
  test_apply_env();
  return 0;
}

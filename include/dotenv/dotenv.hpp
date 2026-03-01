/**
 * @file dotenv.hpp
 * @brief Minimal .env loader for modern C++ (key=value parsing, quotes, comments).
 *
 * `dotenv` provides small, explicit helpers to parse a ".env" file and return
 * a map of variables. It can also optionally apply variables to the process
 * environment (setenv / _putenv_s).
 *
 * Supported:
 * - KEY=VALUE
 * - whitespace trimming
 * - comments starting with '#'
 * - quoted values: "value" or 'value'
 * - basic escapes inside double-quotes: \n \t \r \\ \"
 *
 * Not supported (by design, keep it minimal):
 * - export KEY=VALUE
 * - multiline values
 * - variable expansion ${X}
 *
 * Design goals:
 * - deterministic behavior
 * - minimal API surface
 * - header-only, zero dependencies
 *
 * Requirements: C++17+
 */

#ifndef DOTENV_DOTENV_HPP
#define DOTENV_DOTENV_HPP

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace dotenv
{
  using env_map = std::unordered_map<std::string, std::string>;

  namespace detail
  {
    inline bool is_space(unsigned char c) { return std::isspace(c) != 0; }

    inline void ltrim_inplace(std::string &s)
    {
      std::size_t i = 0;
      while (i < s.size() && is_space(static_cast<unsigned char>(s[i])))
      {
        ++i;
      }
      if (i > 0)
      {
        s.erase(0, i);
      }
    }

    inline void rtrim_inplace(std::string &s)
    {
      if (s.empty())
      {
        return;
      }
      std::size_t i = s.size();
      while (i > 0 && is_space(static_cast<unsigned char>(s[i - 1])))
      {
        --i;
      }
      if (i < s.size())
      {
        s.erase(i);
      }
    }

    inline void trim_inplace(std::string &s)
    {
      ltrim_inplace(s);
      rtrim_inplace(s);
    }

    inline std::string unescape_double_quoted(const std::string &in)
    {
      std::string out;
      out.reserve(in.size());

      for (std::size_t i = 0; i < in.size(); ++i)
      {
        const char ch = in[i];
        if (ch == '\\' && (i + 1) < in.size())
        {
          const char nx = in[i + 1];
          switch (nx)
          {
          case 'n':
            out.push_back('\n');
            break;
          case 't':
            out.push_back('\t');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case '\\':
            out.push_back('\\');
            break;
          case '"':
            out.push_back('"');
            break;
          default:
            out.push_back(nx);
            break;
          }
          ++i;
          continue;
        }
        out.push_back(ch);
      }

      return out;
    }

    inline std::string strip_inline_comment(std::string s)
    {
      // Minimal rule: if value is unquoted, a '#' starts a comment.
      // If value is quoted, we keep '#' inside.
      bool in_single = false;
      bool in_double = false;

      for (std::size_t i = 0; i < s.size(); ++i)
      {
        const char ch = s[i];
        if (ch == '\'' && !in_double)
        {
          in_single = !in_single;
        }
        else if (ch == '"' && !in_single)
        {
          in_double = !in_double;
        }
        else if (ch == '#' && !in_single && !in_double)
        {
          s.erase(i);
          break;
        }
      }

      return s;
    }

    inline bool is_valid_key_char(char c)
    {
      const unsigned char uc = static_cast<unsigned char>(c);
      return (std::isalnum(uc) != 0) || c == '_' || c == '.';
    }

    inline bool is_valid_key(const std::string &key)
    {
      if (key.empty())
      {
        return false;
      }
      for (char c : key)
      {
        if (!is_valid_key_char(c))
        {
          return false;
        }
      }
      return true;
    }

    inline bool parse_line_kv(const std::string &raw_line, std::string &out_key, std::string &out_value)
    {
      std::string line = raw_line;

      // Drop CR (Windows)
      if (!line.empty() && line.back() == '\r')
      {
        line.pop_back();
      }

      trim_inplace(line);
      if (line.empty())
      {
        return false;
      }
      if (!line.empty() && line[0] == '#')
      {
        return false;
      }

      // Find '='
      const std::size_t eq = line.find('=');
      if (eq == std::string::npos)
      {
        return false;
      }

      std::string key = line.substr(0, eq);
      std::string value = line.substr(eq + 1);

      trim_inplace(key);
      trim_inplace(value);

      if (!is_valid_key(key))
      {
        return false;
      }

      // Remove inline comments (only when not inside quotes)
      value = strip_inline_comment(std::move(value));
      trim_inplace(value);

      // Unquote if quoted
      if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')))
      {
        const char q = value.front();
        std::string inner = value.substr(1, value.size() - 2);

        if (q == '"')
        {
          inner = unescape_double_quoted(inner);
        }
        // For single quotes: no escapes, raw inner kept.

        out_key = std::move(key);
        out_value = std::move(inner);
        return true;
      }

      out_key = std::move(key);
      out_value = std::move(value);
      return true;
    }

    inline bool set_process_env(const std::string &key, const std::string &value, bool overwrite)
    {
#if defined(_WIN32)
      // _putenv_s always overwrites, so we simulate overwrite=false by checking.
      if (!overwrite)
      {
        const char *existing = std::getenv(key.c_str());
        if (existing != nullptr)
        {
          return true;
        }
      }
      return _putenv_s(key.c_str(), value.c_str()) == 0;
#else
      // setenv returns 0 on success
      const int ow = overwrite ? 1 : 0;
      return ::setenv(key.c_str(), value.c_str(), ow) == 0;
#endif
    }
  } // namespace detail

  /**
   * @brief Parse a .env file and return a map.
   *
   * @param path Path to the .env file.
   * @throws std::runtime_error on file open failure.
   */
  inline env_map load(const std::string &path)
  {
    std::ifstream in(path);
    if (!in)
    {
      throw std::runtime_error("dotenv: failed to open file: " + path);
    }

    env_map out;
    std::string line;
    while (std::getline(in, line))
    {
      std::string key;
      std::string value;
      if (detail::parse_line_kv(line, key, value))
      {
        out.emplace(std::move(key), std::move(value));
      }
    }

    return out;
  }

  /**
   * @brief Parse a .env file and merge it into an existing map.
   *
   * @param out Map to fill/merge into.
   * @param path Path to the .env file.
   * @param overwrite If false, keep existing keys in the map.
   * @throws std::runtime_error on file open failure.
   */
  inline void load_into(env_map &out, const std::string &path, bool overwrite = true)
  {
    std::ifstream in(path);
    if (!in)
    {
      throw std::runtime_error("dotenv: failed to open file: " + path);
    }

    std::string line;
    while (std::getline(in, line))
    {
      std::string key;
      std::string value;
      if (!detail::parse_line_kv(line, key, value))
      {
        continue;
      }

      if (!overwrite)
      {
        if (out.find(key) != out.end())
        {
          continue;
        }
      }

      out[std::move(key)] = std::move(value);
    }
  }

  /**
   * @brief Get a value from a parsed map.
   */
  inline const std::string &get_or(const env_map &m, const std::string &key, const std::string &fallback)
  {
    const auto it = m.find(key);
    if (it == m.end())
    {
      return fallback;
    }
    return it->second;
  }

  /**
   * @brief Get an environment variable from the process.
   *
   * @return empty string if not found.
   */
  inline std::string getenv_str(const std::string &key)
  {
    const char *v = std::getenv(key.c_str());
    return (v != nullptr) ? std::string(v) : std::string{};
  }

  /**
   * @brief Apply a parsed map into the process environment (setenv/_putenv_s).
   *
   * @param m Parsed env map.
   * @param overwrite If false, existing environment variables are preserved.
   * @throws std::runtime_error if a variable cannot be set.
   */
  inline void apply(const env_map &m, bool overwrite = true)
  {
    for (const auto &kv : m)
    {
      if (!detail::set_process_env(kv.first, kv.second, overwrite))
      {
        throw std::runtime_error("dotenv: failed to set env var: " + kv.first);
      }
    }
  }

  /**
   * @brief Load a .env file and apply it to the process environment.
   *
   * @param path Path to the .env file.
   * @param overwrite If false, existing environment variables are preserved.
   */
  inline void load_and_apply(const std::string &path, bool overwrite = true)
  {
    const env_map m = load(path);
    apply(m, overwrite);
  }

} // namespace dotenv

#endif // DOTENV_DOTENV_HPP

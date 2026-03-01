#include <dotenv/dotenv.hpp>

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
  const std::string path = (argc >= 2) ? std::string(argv[1]) : std::string(".env");

  try
  {
    const dotenv::env_map m = dotenv::load(path);

    const std::string fallback = "default_value";
    const std::string &v = dotenv::get_or(m, "APP_NAME", fallback);

    std::cout << "APP_NAME=" << v << "\n";
  }
  catch (const std::exception &e)
  {
    std::cerr << "dotenv example error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

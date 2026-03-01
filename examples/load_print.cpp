#include <dotenv/dotenv.hpp>

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
  const std::string path = (argc >= 2) ? std::string(argv[1]) : std::string(".env");

  try
  {
    const dotenv::env_map m = dotenv::load(path);

    std::cout << "Loaded " << m.size() << " vars from: " << path << "\n";
    for (const auto &kv : m)
    {
      std::cout << kv.first << "=" << kv.second << "\n";
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "dotenv example error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

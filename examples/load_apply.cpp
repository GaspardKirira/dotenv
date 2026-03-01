#include <dotenv/dotenv.hpp>

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
  const std::string path = (argc >= 2) ? std::string(argv[1]) : std::string(".env");

  try
  {
    dotenv::load_and_apply(path, true);

    std::cout << "Applied .env to process environment from: " << path << "\n";
    std::cout << "Tip: use dotenv::getenv_str(\"KEY\") to read values.\n";
  }
  catch (const std::exception &e)
  {
    std::cerr << "dotenv example error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

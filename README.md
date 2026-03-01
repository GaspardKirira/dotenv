# dotenv

Minimal .env loader for modern C++.

`dotenv` provides deterministic helpers to parse and load environment
variables from `.env` files.

It supports simple `KEY=VALUE` parsing with trimming, quoting, and
comment handling.

Header-only. Zero external dependencies.

---

## Download

https://vixcpp.com/registry/pkg/gaspardkirira/dotenv

---

## Why dotenv?

Configuration files appear everywhere:

- Web servers
- CLI tools
- Microservices
- Local development environments
- CI/CD pipelines

Reading `.env` files manually often leads to:

- Inconsistent whitespace handling
- Incorrect comment stripping
- Broken quote parsing
- Missing escape support
- Repeated boilerplate

This library provides:

- Deterministic `KEY=VALUE` parsing
- Whitespace trimming
- Inline comment handling
- Support for single and double quotes
- Basic escape handling (`\n`, `\t`, `\r`, `\\`, `\"`)
- Optional process environment application

No heavy parser.
No macro system.
No variable expansion engine.

Just minimal `.env` loading primitives.

---

## Installation

### Using Vix Registry

```bash
vix add gaspardkirira/dotenv
vix deps
```

### Manual

```bash
git clone https://github.com/GaspardKirira/dotenv.git
```

Add the `include/` directory to your project.

---

## Dependency

Requires C++17 or newer.

No external dependencies.

---

## Quick Examples

### Load and Inspect

```cpp
#include <dotenv/dotenv.hpp>
#include <iostream>

int main()
{
    auto env = dotenv::load(".env");

    for (const auto& kv : env)
        std::cout << kv.first << "=" << kv.second << "\n";
}
```

### Load and Apply to Process Environment

```cpp
#include <dotenv/dotenv.hpp>

int main()
{
    dotenv::load_and_apply(".env");

    std::string port = dotenv::getenv_str("PORT");
}
```

### Safe Fallback Access

```cpp
#include <dotenv/dotenv.hpp>
#include <iostream>

int main()
{
    auto env = dotenv::load(".env");

    const std::string fallback = "default";
    const std::string& value = dotenv::get_or(env, "APP_NAME", fallback);

    std::cout << value << "\n";
}
```

---

## API Overview

```cpp
dotenv::load(path);
dotenv::load_into(map, path, overwrite);

dotenv::apply(map, overwrite);
dotenv::load_and_apply(path, overwrite);

dotenv::get_or(map, key, fallback);
dotenv::getenv_str(key);
```

---

## Supported Syntax

```env
# Comment
FOO=bar
HELLO = world

QUOTED1="hello world"
QUOTED2='raw value'

ESCAPED="line1\nline2"
INLINE=value # comment
EMPTY=
```

Rules:

- Leading/trailing whitespace is trimmed.
- `#` starts a comment unless inside quotes.
- Double quotes support basic escapes.
- Single quotes are treated as raw strings.
- Invalid keys are ignored.
- No variable expansion.
- No multiline values.

---

## Complexity

Let:

- L = number of lines
- C = total characters

| Operation      | Time Complexity |
|---------------|----------------|
| Parsing file  | O(C)           |
| Map insertion | O(L)           |

Memory usage is proportional to the total parsed content.

---

## Semantics

- Duplicate keys overwrite previous values (unless `overwrite=false`).
- `load()` throws if file cannot be opened.
- `apply()` throws if setting an environment variable fails.
- Deterministic behavior given deterministic input.

---

## Design Principles

- Explicit over implicit
- Minimal parsing surface
- Deterministic semantics
- No hidden magic
- No runtime surprises

This library provides primitives only.

If you need:

- Variable interpolation `${VAR}`
- Multiline values
- Advanced config layering
- Schema validation

Build them on top of this layer.

---

## Tests

```bash
vix build
vix tests
```

Tests verify:

- Basic parsing
- Quotes and escapes
- Inline comments
- Merge behavior
- Environment application

---

## License

MIT License\
Copyright (c) Gaspard Kirira


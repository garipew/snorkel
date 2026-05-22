# snorkel
> A collection of low-level systems experiments in C, focused on implementing core primitives such as memory allocators, coroutines and thread pools.

---

## About

Snorkel was designed as a monolith library to explore advanced topics on systems programming. After some time, a design shift happened in favor of a more ergonomic style, a collection of header-only libraries. 

These are the topics explored on snorkel:
- Memory allocator (arena style)
- Coroutines
- Thread pool

And these are the C obscure techniques used:
- Stb-style header-only libraries
- Optional arguments with macros
- Inline assembly

## Design Philosophy

Snorkel implements a memory allocator on `snorkel_arena.h` but does not force it. The stb design makes snorkel a modular library, no dependencies over each of snorkel pieces. That shouldn't mean, and it doesn't, that you can't use snorkel arenas on the other modules. Snorkel is allocator agnostic, to initialize modules, inject an allocator.

## Getting started

To get snorkel, clone this repo with

```sh
git clone https://github.com/garipew/snorkel
cd snorkel
```

As every other header-only library, any header file on snorkel can simply be dropped into your project and included as a normal header files.

```c
#include "snorkel_arena.h"
// Or
#include "snorkel_pool.h"
// Or
#include "snorkel_co.h"
```

Before their last include, make sure to define `SNORKEL_IMPLEMENTATION`:

```c
#define SNORKEL_IMPLEMENTATION
#include "snorkel_arena.h"
#include "snorkel_pool.h"
#include "snorkel_co.h"
```

## Notes

Details, references and inspirations for snorkel:
- stb header libraries
- Fast Allocation and Deallocation of Memory Based on Object Lifetimes (David R. Hanson)
- Tsoding

---

## Contributing

Issues and pull requests are welcomed.

## License

This project is licensed under MIT license.

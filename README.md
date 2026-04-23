# snorkel
> A low-level playground for implementation of any interesting concept that I come across.

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

## Getting started

To get snorkel, clone this repo with

```sh
git clone https://github.com/garipew/snorkel
cd snorkel
```

As every other header-only library, snorkel_arena.h and snorkel_pool.h can simply be dropped into your project and included as a normal header files.

```c
#include "snorkel_arena.h"
// And
#include "snorkel_pool.h"
```

Before their last include, make sure to define `SNORKEL_IMPLEMENTATION`:

```c
#define SNORKEL_IMPLEMENTATION
#include "snorkel_arena.h"
#include "snorkel_pool.h"
```

The coroutines are still implemented over the legacy snorkel version, which means in order to use them, you'll have to compile snorkel. Thankfully a Makefile was written to help with this process.  

```sh
make
# And
sudo make install
```

## What's next

Snorkel does not have a clear goal, neither it has a consistent development rhythm. That said, there will always be interesting topics left to explore and when diving deeper, a snorkel can be handy!

With that in mind, there's still at least some tangible milestone left:
    - [ ] Finish thread pool
    - [ ] Refactor with the goal of removing **every** hidden allocation
    - [ ] Refactor with the goal of fully turning snorkel into a collection of header-only libraries

---

## Contributing

Issues and pull requests are welcomed.

## Notes

Details, references and inspirations for snorkel:
    - stb header libraries
    - win32 API
    - Fast Allocation and Deallocation of Memory Based on Object Lifetimes (David R. Hanson)
    - Tsoding

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

Snorkel is intentionally not uniform. Each module explores a different approach to API design, particularly around memory ownership:

- Coroutines (`snorkel_co.h`)
Uses allocator injection. User register allocation functions, allowing the library to remain flexible while retaining control over internal memory usage.

- Thread pool (`snorkel_pool.h`)
Avoid implicit allocations entirely. Functions that require memory receive it explicitly from the caller, making ownership and lifetime fully transparent.

These contrasting approaches are deliberate, allowing exploration of tradeoffs between:

- control vs ergonomics
- explicitness vs convenience
- abstraction vs predictability

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

## What's next

Snorkel does not have a clear goal, neither it has a consistent development rhythm. That said, there will always be interesting topics left to explore and when diving deeper, a snorkel can be handy!

With that in mind, there's still at least some tangible milestone left:
- [ ] Finish thread pool

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

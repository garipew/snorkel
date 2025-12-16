# snorkel
This repo groups in a single place every new concept I find interesting enough
to implement.

It is all very experimental, and you should not expect a production-ready
general use library. That being said, feel free to take a look and help me out,
I hope I am not commiting any grotesque error, but as they say, two heads think
better than one. Your help will be much apreciated! :)

```
make
```

And
```
sudo make install
```

## Arenas
Arenas were the starting point of snorkel, it was very fun to implement and so
rewarding! Never again I am going back to the malloc/free nightmare...

My implementation is still very concise, but it is slowly growing as my needs
also grow.

## strings
Well, since I was already here, there's no harm in packing the length with the
char\*. Also included some helper functions, but didn't really go too far on
this one.

## coroutines
It's in C, I swear!!!!

I never really understood the application of coroutines, until I started making
my [chip8 emulator](https://github.com/garipew/cemu8) and decided to play
around with asynchronous IO operations. And then, all the potential struck me,
so, as any normal person would, I decided to give it a try and implement it!

## tests
Since I am already doing all this, I shall seize the opportunity to delve into
testing

Run the tests with
```
make test
```

## Notes
I intend to let this repo grow as time passes, slowly.

### About threading...
As far as I can tell, snorkel is thread-safe.

When using coroutines on multiple threads, each thread requires it's own
scheduler and arena. However, I don't know how to test thread-safety and
there could still be gotchas I am missing.

### On memory
You can define which Arena the coroutine will use by passing it to the
functions coroutine_create, coroutine_start, coroutine_step and
coroutine_collect.

Arenas are never implicitly free'd. The arena passed to coroutine_start is
resetted at it's end and it's highly encouraged to manually do the same when
reaching the end of execution via coroutine_step.

Use coroutine_collect() to free the allocated memory. In the case of usage of a
custom arena:
```
coroutine_collect(.arena=&custom_arena);
```

### On stepping
The only way to know if a coroutine reached it's end is when coroutine_step
returns NULL, which is not ideal, since NULL could be a valid value to yield.

In the future, maybe coroutine_step could also pack this info.

That's all for now!

When starting to dive a little bit deeper, a snorkel can be handy! :)

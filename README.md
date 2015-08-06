zeta
====

This is an implementation of a Virtual Machine (VM) for the zeta programming language
I'm working on in my space time. The language is currently at the early prototype stage.
It will be dynamically typed and feature a user-extensible grammar. Use of
functional-style programming will be encouraged by the language, but not mandated.

I've chosen to implement the VM core in pure C, for the following reasons:

- To make low-level details explicit (for instance, the layout of hosted objects in memory)

- To avoid hidden sources of overhead

- To avoid dependence on non-portable tools and languages. GCC is available on almost every platform in existence

The zeta implementation will be largely self-hosted. The core VM will
implement an interpreter in C, but the garbage collector and zeta JIT
compiler will be written in the zeta language itself.


zeta
====

This is an implementation of a Virtual Machine (VM) for the zeta programming language
I'm working on in my space time. The language is currently at the early prototype stage.
It will be dynamically typed and feature a user-extensible grammar.

I've chosen to implement the VM core in pure C so as to make low-level details explicit
(for instance, the layout of hosted objects in memory). Much of the implementation of
zeta will be self-hosted. The core VM will implement an interpreter, but the garbage
collector and zeta JIT compiler will be written in the zeta language itself.

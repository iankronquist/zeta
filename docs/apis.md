Zeta Core APIs
==============

The core APIs are functions written in C and exposed to Zeta through wrappers
which provide means for applications to interface with the outside world
without the use of a Foreign Function Interface (FFI). These deal only with
low-level data types and are not means to be used directly to write
applications, instead, user-friendly libraries will be witten on top of the
APIs and exposed as Zeta modules. The aim is to provide a standard set of
essential and useful functionality in a cross-platform manner. The core APIs
are intended to change as little as possible and be implementable on every
platform.


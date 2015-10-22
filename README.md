The Zeta Programming Language
=============================

[![Build Status](https://travis-ci.org/maximecb/zeta.png)](https://travis-ci.org/maximecb)

This is an implementation of a Virtual Machine (VM) for the Zeta programming
language I'm working on in my spare time. Zeta draws inspiration from LISP,
Smalltalk, ML, Python, JavaScript and C. The language is currently at the early
prototype stage, meaning the syntax and semantics are likely to change a lot
in the coming months.

At the moment, this platform and language is mostly of interest to tinkerers
and those with a special interest in compilers or language design. It's still
much too early to talk about adoption and long term goals. I'll be happy if
this project can help me and others learn more about language and compiler
design.

## Quickstart

Requirements:

- A C compiler, GCC 4.8+ recommended (clang and others untested)

- GNU make

- A POSIX compliant OS (Linux or Mac OS)

To built the Zeta VM, go to the source directory and run `make`

Tests can then be executed by running the `make test` command

A read-eval-print loop (shell) can be started by running the `./zeta` binary

## Language Design

Planned features of the Zeta programming language include:

- Dynamic typing

- Garbage collection

- Dynamically extensible objects with prototypal inheritance, as in JavaScript

- No distinction between statements and expression, everything is an expression, as in LISP

- A user-extensible grammar, giving programmers the ability to define new syntactic constructs

- Operator overloading, to allow defining new types that behave like native types

- Functional constructs such as map and foreach

- A module system

- A very easy to use canvas library to render simple 2D graphics and make simple UIs

- The ability to suspend and resume running programs

Zeta takes inspiration from JavaScript, but notable differences include:

- No `undefined` or `null` values

- No `new` keyword or constructor functions

- Object properties cannot be deleted

- Arrays cannot have "holes" (missing elements)

- Attempting to read missing object properties throws an exception

- Distinct 64-bit integer and floating-point value types

- 64-bit integers overflow as part of normal semantics

- Arithmetic operators do not accept strings as input values

- Global variables are not shared among different source files

- The `eval` function cannot access local variables

- A distinction between `print` and `println` functions

If you want to know more about my views on programming language design, I've
written several
[blog posts on the topic](http://pointersgonewild.com/category/programming-languages/).

## Zeta Core Language Syntax

The syntax of the Zeta programming language is not finalized. The language is
designed to be easy to parse (no backtracking or far away lookup), relatively
concise, easy to read and familiar-seeming to most experienced programmers.
In Zeta, every syntactic construct is an expression which has a value (although
that value may have no specific meaning in some cases).

The Zeta grammar will be extensible. The Zeta core language itself (without
extensions), is going to be kept intentionally simple and minimalistic.
Features such as regular expressions, switch statements, pattern matching and
template strings will be implemented as grammar extension in libraries, and
not part of the core language. The advantage here is that the core VM will not
need to know anything about things such as regular expressions, and multiple
competing regular expression packages can be implemented for Zeta.

Here is an example of what Zeta code might look like:

```
/*
Load/import the standard IO module
Modules are simple objects with properties
*/
io = import("io")

io.println("This is an example Zeta script");

// Fibonacci function
let fib = fun (n) if n < 1 then n else fib(n-1) + fib(n-1)

// Compute the meaning of life and print out the answer
io.println(fib(42))

// This is a global variable declaration
var y;

let foo = fun (n)
{
    io.println("It's also possible to execute expressions in sequence");
    io.println("inside blocks with curly braces.");

    // Since we have parenthesized expressions, we could almost pretend
    // This is JavaScript code, except for the lack of semicolons
    if (n < 1) then
    {
        io.println("n is less than 1")
    }
    else
    {
        io.println("n is greater than or equal to 1")
    }

    // This is a local constant declaration, x cannot be reassigned to
    let x = 7 + 1

    // This assigns to the global variable y
    y = 3

    // We can also create anonymous closures
    let bar = fun () x + y

    // This function returns the closure bar, the last expression we
    // have evaluated
}

// This is an object literal
let obj = { x:3, y:5 }

// When declaring a method, the "this" argument is simply the first
// function argument, and you can give it the name you want, avoiding all
// of the JavaScript "this" issues
obj.method = fun (this, x) this.x = x

// This object inherits from obj using prototypal inheritance
let obj2 = obj::{ y:6, z:7 }

// The language suppports arrays with zero-based indexing
let arr = [0, 1, 2, obj, obj2]

// Make the fib and foo functions available to other modules.
export('fib', fib)
export('foo', foo)
```

Everything is still in flux. Your comments on the syntax and above
example are welcome.

One issue I'm aware of that may or may not be problematic in practice is
that the parser is greedy, and the lack of semicolons could make it slightly
confusing to people. For instance:

```
// This is a sequence of two expressions, x then y
x y

// This is x+y and then z+w in a sequence
x+y
z+w

// But! The following is interpreted as a function call because of
// the second set of parentheses
(x+y)
(z+w) 
```

We may or may not want to use semicolons to separate expressions inside
sequences (at the top-level of scripts and inside curly braces). The question
would then be whether semicolons should be optional or mandatory. I expect
such issues will be easier to work out once we start writing more Zeta code
and dogfooding the system.

## Language Extensions

Zeta's "killer feature" will be the ability to extend the language in a way that feels native. The core language will be kept lean and minimal, but a library of "officially sanctioned" language extensions will be curated. For instance, the core Zeta language will only support functions with fixed arity and positional arguments. I would like, however, to implement variable arity functions, default values, optional and keyword arguments (as in Python) as a language extension.

Some useful language extensions I can think of

- Variable arity functions, optional arguments, keyword arguments
- Multiple return values
- Dictionary types
- JS-like regular expressions
- JS-like template strings
- PHP-like templating mode
- MATLAB-like vector and matrix arithmetic

## Zeta VM

I've chosen to implement the VM core in pure C (not C++), for the following reasons:

- To make low-level details explicit (for instance, the layout of hosted objects in memory)

- To avoid hidden sources of overhead

- To maximize portability. GCC is available on almost every platform in existence.

An important goal of the Zeta VM is that it should be easy to build on both
Mac and Linux with minimal dependencies. You may need to install libraries to
use graphical capabilities and such, but the core VM should always with only a
C compiler installed on your machine. Zeta must alway remain easy to install and
get started with.

The zeta implementation will be largely [self-hosted](https://en.wikipedia.org/wiki/Self-hosting).
The core VM will implement an interpreter in C, but the garbage collector and Zeta JIT
compiler will be written in the Zeta language itself.

A custom x86 assembler and backend will be built for the JIT compiler, as was done
for the [Higgs](https://github.com/higgsjs/Higgs) VM. This will allow self-modifying
code to be generated by the JIT. The x86 backend will be implemented in the Zeta language as well.

## Plan of Action

I plan to bootstrap Zeta in several stages. The first step is to write a
parser for the Zeta core language in C. This is already largely done. The core
parser will have limited capabilities and will not feature an extensible
grammar, as it will only serve to bootstrap the system.

The second step of the project is to implement an interpreter for the Zeta
core language. This interpreter will also have limited language support. The
point of the interpreter is to eventually interpret the code for the Zeta
JIT compiler, which will itself be written in Zeta. Because the core
interpreter is only a stepping stone in the bootstrap process, I intend to cut
corners, keeping the implementation minimal and restricted in terms of
supported language features. For instance, I believe it should be possible
to bootstrap Zeta without a garbage collector. At the time of this writing,
the interpreter is a work in progress.

In order to permit grammar extensions, we will implement a Zeta parser in
Zeta itself. This parser will run on the Zeta core interpreter. Implementing
the parser in Zeta will allow us to make use of features not found in C such
as extensible objects, garbage collection, closures, etc. It will also make it
much easier for Zeta code to interface with the parser in order to extend the
grammar. I intend to implement the self-hosted, extensible Zeta parser soon
after the interpreter is ready, so that we can showcase the power of the
extensible Zeta grammar.

The Zeta JIT compiler will be written in Zeta and initially interpreted.
The goal is for the JIT compiler to be able to compile itself to
native x86 machine code. It will also be able to JIT compile the self-hosted
Zeta parser. Once these components are compiled to native code, the system
should run at an acceptable speed. Note that the first version of
the Zeta JIT compiler is likely to be far from the performance-level
of modern JavaScript JITs. I do believe, however, that we can very easily beat
the performance of the stock CPython and Ruby implementations, even with a
naive JIT.

You may be thinking that the startup of the Zeta VM will be dog slow if we
use an interpreted JIT compiler which compiles itself, the Zeta runtime, and our
self-hosted Zeta parser, all of this before any user code can be run. This
bootstrap phase is likely to take up to a few minutes. This is
where the ability to serialize an initialized heap to an image on disk will
come in handy. Because all of the Zeta data structures are self-hosted, we will
be able to save the initialized heap to disk after the bootstrap/initialization
is complete. Then, restarting the VM from this saved point should only take
milliseconds.

If you would like to contribute or get a better idea of tasks to be completed,
take a look at the list of [open issues](https://github.com/maximecb/zeta/issues).


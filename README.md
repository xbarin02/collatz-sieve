# collatz-sieve
This repository contains convergence sieves for the verification of the Collatz problem.

To understand the `*.map` files, see [the detailed description on the Wikipedia page](https://en.wikipedia.org/wiki/Collatz_conjecture#Modular_restrictions).

For example, the 16 least-significant bits of a number `n` can be used as bit-index into the `sieve-16.map`.
This result is that only 2114 out of 65536 numbers (about 3.2 %) must be checked for convergence.

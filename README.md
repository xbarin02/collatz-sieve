# collatz-sieve
This repository contains convergence sieves for the verification of the Collatz problem.

To understand the `*.map` files, see [the detailed description on the Wikipedia page](https://en.wikipedia.org/wiki/Collatz_conjecture#Modular_restrictions).

For example, the 16 least-significant bits of a number `n` can be used as bit-index into the `esieve-16.map`.
This result is that only 1720 out of 65536 numbers (about 2.6 %) must be checked for convergence.

I have found that these convergence sieves (considering values stored in bits) have a kind of internal structure.
Specifically, sieves are formed by only several constantly repeated 64-bit patterns, so I store only indices into a small look-up table.

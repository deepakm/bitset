# TODO

 - Create bitset_vector_verify() to check vector buffer integrity
 - Encode lengths > 2^26 using a fill with P=0 followed by another. Length is L1 << 26 & L2
 - Return NULL / error code instead of dying on oom
 - Implement probabilistic HyperLogLog and Top-k algorithms for streams of bitsets
 - Ensure const-correctness
 - Use builtin ffs(), fls(), POPCNT if available
 - Reduce duplicate code (bitset{_vector,}_hash, buffers with pow2 resizing)
 - Investigate behavior on illumos/smartos (read: its broken)
 - Create macros for echo output
 - Make newstyle compile output optional
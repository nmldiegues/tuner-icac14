Tuner Algorithm for Intel TSX HTM
============

*Self-Tuning Intel Transactional Synchronization Extensions*

Transactional Memory was recently integrated in Intel processors under the name TSX. 
This Hardware Transactional Memory (HTM) implementation requires a software-based fallback and, in fact, its performance depends significantly on that interplay.
Wide experimentation showed that there does not seem to exist a single configuration for that fallback that can perform best independently of the application and workload.

For this, Tuner was devised as a self-tuning approach that exploits lightweight reinforcement learning techniques to identify the optimal TSX configuration in a workload-oblivious manner, i.e., not requiring any off-line sampling of the application.

The folder "tuner-gcc" contains an implementation of Tuner inside GCC 4.8.2 and the benchmarks relying on the C++ TM Standard.
The folder "tuner-selective" contains a library-based implementation of Tuner following the typical approach in STAMP backends. For this, the benchmarks were manually instrumented to invoke the macros that direct to the library.

For questions, contact the author:
Nuno Diegues - nmld@tecnico.ulisboa.pt

When using this work, please cite accordingly: 
 Nuno Diegues and Paolo Romano, "Self-Tuning Intel Transactional Synchronization Extensions", Proceedings of the International Conference on Autonomic Computing, ICAC 2014

You may also find the following paper relevant for citation:
 Nuno Diegues, Paolo Romano, and Luis Rodrigues, "Virtues and Limitations of Commodity Hardware Transactional Memory", Proceedings of the International Conference on Parallel Architectures and Compiler Techniques, PACT 2014

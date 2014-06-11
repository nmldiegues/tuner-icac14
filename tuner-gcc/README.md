Instructions:
 - download GCC 4.8.2 (e.g.: https://github.com/mirrors/gcc/releases)
 - compile GCC, which will entail following its README and installing its dependencies
 - swap a couple files (residing in "gcc-modified-files") that were modified to implement Tuner in GCC: 
   * "beginend.cc" has to be moved into "gcc-4.8.2/libitm/"
   * "trans-mem.h/c" have to be moved inside "gcc-4.8.2/gcc/"
   * diff these files with the original ones to see where things changed
 - recompile GCC
 - fix the paths in the file "common/Defines.common.mk" to point to your GCC folder
 - run the script build-itm.sh
 - you should now find the binaries for each benchmark inside the respective folders

For questions, contact the author:
Nuno Diegues - nmld@tecnico.ulisboa.pt

When using this work, please cite accordingly: 
 Nuno Diegues and Paolo Romano, "Self-Tuning Intel Transactional Synchronization Extensions", Proceedings of the International Conference on Autonomic Computing, ICAC 2014

You may also find the following paper relevant for citation:
 Nuno Diegues, Paolo Romano, and Luis Rodrigues, "Virtues and Limitations of Commodity Hardware Transactional Memory", Proceedings of the International Conference on Parallel Architectures and Compiler Techniques, PACT 2014

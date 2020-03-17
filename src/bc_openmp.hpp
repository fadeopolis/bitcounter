
/// Helper header for making it easy to enable/disable OpenMP macros without lots of #ifdefs
/// Yes, OpenMP pragmas are ignored when compiling without -fopenmp ...
/// but GCC/Clang give lots of warnings, and I really like -Wall and dislike disabling single warnings.
/// I've seen that snowball too many times

#define BC_STRINGIFY_(X) #X
#define BC_STRINGIFY(X)  BC_STRINGIFY_(X)

#ifdef _OPENMP
#  define BC_OMP(X) _Pragma(BC_STRINGIFY(omp X))
#else
#  define BC_OMP(X)
#endif

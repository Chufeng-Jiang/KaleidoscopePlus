#include "kaleidoscope/runtime/ArrayABI.h"
#include <cstdlib>
static inline double element(double x,double y,double a) { return (x*a+y); }
extern "C" void checked(KaleArray *x,KaleArray *y,KaleArray *out,double a) {
  for(int i=0;i<x->sizes[0];++i) {
    if(i>=y->sizes[0] || i>=out->sizes[0]) std::abort();
    out->aligned[out->offset+i]=element(x->aligned[x->offset+i],y->aligned[y->offset+i],a);
  }
}
extern "C" void unchecked(KaleArray *x,KaleArray *y,KaleArray *out,double a) {
  for(int i=0;i<x->sizes[0];++i)
    out->aligned[out->offset+i]=element(x->aligned[x->offset+i],y->aligned[y->offset+i],a);
}

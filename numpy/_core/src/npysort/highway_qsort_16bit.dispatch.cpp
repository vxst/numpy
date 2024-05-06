#include "highway_qsort.hpp"
#define VQSORT_ONLY_STATIC 1
#include "hwy/contrib/sort/vqsort-inl.h"

#include "quicksort.hpp"

namespace np { namespace highway { namespace qsort_simd {

template<> void NPY_CPU_DISPATCH_CURFX(QSort)(Half *arr, intptr_t size)
{
#if HWY_HAVE_FLOAT16
    hwy::HWY_NAMESPACE::VQSortStatic(reinterpret_cast<hwy::float16_t*>(arr), size, hwy::SortAscending());
#else
    sort::Quick(arr, size);
#endif
}
template<> void NPY_CPU_DISPATCH_CURFX(QSort)(uint16_t *arr, intptr_t size)
{
    hwy::HWY_NAMESPACE::VQSortStatic(arr, size, hwy::SortAscending());
}
template<> void NPY_CPU_DISPATCH_CURFX(QSort)(int16_t *arr, intptr_t size)
{
    hwy::HWY_NAMESPACE::VQSortStatic(arr, size, hwy::SortAscending());
}

} } } // np::highway::qsort_simd

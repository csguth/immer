//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <nonius.h++>

#include <immer/heap/gc_heap.hpp>
#include <immer/memory_policy.hpp>

namespace {

NONIUS_PARAM(N, std::size_t{1000})

struct gc_disable
{
    gc_disable()
    {
#if IMMER_BENCHMARK_DISABLE_GC
        GC_disable();
#else
        GC_gcollect();
#endif
    }
    ~gc_disable()
    {
#if IMMER_BENCHMARK_DISABLE_GC
        GC_enable();
        GC_gcollect();
#endif
    }
    gc_disable(const gc_disable&) = delete;
    gc_disable(gc_disable&&) = delete;
};

template <typename Meter, typename Fn>
void measure(Meter& m, Fn&& fn)
{
    gc_disable guard;
    return m.measure(std::forward<Fn>(fn));
}

using def_memory    = immer::default_memory_policy;
using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using gcf_memory    = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy, immer::gc_transience_policy, false>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::cpp_heap>, immer::refcount_policy>;
using safe_memory   = immer::memory_policy<immer::free_list_heap_policy<immer::cpp_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::unsafe_free_list_heap_policy<immer::cpp_heap>, immer::unsafe_refcount_policy>;

} // anonymous namespace

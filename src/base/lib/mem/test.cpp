/******************************************************************************
 * Copyright (c) 2010-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "pch.h"

#include <mi/base/assert.h>
#include <base/system/test/i_test_auto_case.h>
#include <base/lib/log/i_log_assert.h>
#include <base/lib/mem/i_mem_aligned.h>
#include <vector>

MI_TEST_AUTO_FUNCTION(test_alignment)
{
    using namespace MI;

    for (size_t a = sizeof(size_t); a <= 8*sizeof(size_t); a += sizeof(size_t))
    {
        std::vector<STLEXT::Shared_array<float> > v;

        for (size_t i = 0; i < 100; ++i)
        {
            const STLEXT::Shared_array<float> p(
                MEM::alloc_aligned_POD<float>(i, a));

            mi_static_assert(sizeof(float*) == sizeof(size_t));
            const size_t address = reinterpret_cast<size_t>(p.get());
            MI_CHECK(!(address % a));

            for (size_t j = 0; j < i; ++j)
                p[j] = float(j);

            v.push_back(p);
        }
    }
}


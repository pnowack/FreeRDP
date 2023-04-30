/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RemoteFX Codec Library - SIMD Optimizations
 *
 * Copyright 2023 Pascal Nowack <Pascal.Nowack@gmx.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <freerdp/config.h>

#include "rfx_simd.h"

#include <winpr/sysinfo.h>

#include "rfx_avx2.h"
#include "rfx_neon.h"
#include "rfx_sse2.h"

void rfx_init_simd(RFX_CONTEXT* rfx_context)
{
	if (FALSE)
		WINPR_ASSERT(FALSE);
#ifdef WITH_AVX2
	else if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) &&
	         IsProcessorFeaturePresentEx(PF_EX_AVX2))
		rfx_init_avx2(rfx_context);
#endif
#ifdef WITH_SSE2
	else if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
		rfx_init_sse2(rfx_context);
#endif
#ifdef WITH_NEON
	else if (IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE))
		rfx_init_neon(rfx_context);
#endif
}

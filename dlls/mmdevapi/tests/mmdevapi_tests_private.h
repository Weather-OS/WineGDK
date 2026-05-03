/*
 * Copyright 2025 Giovanni Mascellani for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"

#include "audioclient.h"

struct wave_format
{
    WAVEFORMATEXTENSIBLE format;
    const char *additional_context;
};

extern struct wave_format *wave_formats;
extern size_t wave_format_count;

inline static void push_format_context(const WAVEFORMATEXTENSIBLE *fmt)
{
    static const char *format_str[] =
    {
        [WAVE_FORMAT_PCM] = "P",
        [WAVE_FORMAT_IEEE_FLOAT] = "F",
        [WAVE_FORMAT_ALAW] = "A",
        [WAVE_FORMAT_MULAW] = "MU",
    };

    if (fmt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        winetest_push_context("%sX%u(%u)x%lux%u:%lx", format_str[fmt->SubFormat.Data1],
                fmt->Format.wBitsPerSample, fmt->Samples.wValidBitsPerSample,
                fmt->Format.nSamplesPerSec, fmt->Format.nChannels, fmt->dwChannelMask);
    }
    else
    {
        winetest_push_context("%s%ux%lux%u", format_str[fmt->Format.wFormatTag],
                fmt->Format.wBitsPerSample, fmt->Format.nSamplesPerSec, fmt->Format.nChannels);
    }
}

HRESULT validate_fmt(const WAVEFORMATEXTENSIBLE *fmt, BOOL compatible);
void fill_wave_formats(const WAVEFORMATEXTENSIBLE *base_fmt);

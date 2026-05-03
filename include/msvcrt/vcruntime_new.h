/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#ifndef __VCRUNTIME_NEW_H
#define __VCRUNTIME_NEW_H

#ifdef __cplusplus
extern "C++"
{

namespace std
{
enum class align_val_t : size_t {};
struct nothrow_t {};
extern nothrow_t const nothrow;
} /* namespace std */

void *__cdecl operator new(size_t size);
void *__cdecl operator new[](size_t size);
void __cdecl operator delete(void *ptr) noexcept;
void __cdecl operator delete[](void *ptr) noexcept;
void __cdecl operator delete(void *ptr, size_t size) noexcept;
void __cdecl operator delete[](void *ptr, size_t size) noexcept;

void *__cdecl operator new(size_t size, std::align_val_t align);
void *__cdecl operator new[](size_t size, std::align_val_t align);
void __cdecl operator delete(void *ptr, std::align_val_t align) noexcept;
void __cdecl operator delete[](void *ptr, std::align_val_t align) noexcept;
void __cdecl operator delete(void *ptr, size_t size, std::align_val_t align) noexcept;
void __cdecl operator delete[](void *ptr, size_t size, std::align_val_t align) noexcept;

void *__cdecl operator new(size_t size, std::nothrow_t const&) noexcept;
void *__cdecl operator new[](size_t size, std::nothrow_t const&) noexcept;
void __cdecl operator delete(void *ptr, std::nothrow_t const&) noexcept;
void __cdecl operator delete[](void *ptr, std::nothrow_t const&) noexcept;

void *__cdecl operator new(size_t size, std::align_val_t align, std::nothrow_t const&) noexcept;
void *__cdecl operator new[](size_t size, std::align_val_t align, std::nothrow_t const&) noexcept;
void __cdecl operator delete(void *ptr, std::align_val_t align, std::nothrow_t const&) noexcept;
void __cdecl operator delete[](void *ptr, std::align_val_t align, std::nothrow_t const&) noexcept;

inline void *operator new(size_t size, void *ptr) noexcept { return ptr; }
inline void *operator new[](size_t size, void *ptr) throw() { return ptr; }
inline void operator delete(void*, void*) noexcept {}
inline void operator delete[](void*, void*) noexcept {}

}
#endif /* __cplusplus */

#endif /* __VCRUNTIME_NEW_H */

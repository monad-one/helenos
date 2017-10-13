/*
 * Copyright (c) 2017 Jaroslav Jindrak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBCPP_NEW
#define LIBCPP_NEW

#include <exception>

namespace std
{

class bad_alloc: public std::exception
{
	public:
		bad_alloc() = default;
		bad_alloc(const bad_alloc&);
		bad_alloc& operator=(const bad_alloc&) = default;
		virtual const char* what() const override;
		virtual ~bad_alloc() = default;
};

struct nothrow_t {};
extern const nothrow_t nothrow;

using new_handler = void (*)();

new_handler set_new_handler(new_handler);
new_handler get_new_handler() noexcept;

}

void* operator new(std::size_t);
void* operator new(std::size_t, void*);
void* operator new(std::size_t, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t);
void* operator new[](std::size_t, const std::nothrow_t&) noexcept;

void operator delete(void*) noexcept;
void operator delete(void*, std::size_t) noexcept;
void operator delete[](void*) noexcept;
void operator delete[](void*, std::size_t) noexcept;

#endif


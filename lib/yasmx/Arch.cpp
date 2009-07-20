//
// Architecture
//
//  Copyright (C) 2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "yasmx/Arch.h"

#include "util.h"

#include "llvm/Support/Streams.h"
#include "YAML/emitter.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Insn.h"


namespace yasm
{

Register::~Register()
{
}

void
Register::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::cerr << out.c_str() << std::endl;
}

RegisterGroup::~RegisterGroup()
{
}

void
RegisterGroup::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::cerr << out.c_str() << std::endl;
}

SegmentRegister::~SegmentRegister()
{
}

void
SegmentRegister::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::cerr << out.c_str() << std::endl;
}

Arch::InsnPrefix::InsnPrefix(std::auto_ptr<Insn> insn)
    : m_type(INSN),
      m_insn(insn.release())
{}

Arch::InsnPrefix::~InsnPrefix()
{
    if (m_type == INSN)
        delete m_insn;
}

std::auto_ptr<Insn>
Arch::InsnPrefix::ReleaseInsn()
{
    if (m_type != INSN)
        return std::auto_ptr<Insn>(0);
    m_type = NONE;
    return std::auto_ptr<Insn>(m_insn);
}

Arch::~Arch()
{
}

void
Arch::AddDirectives(Directives& dirs, const char* parser)
{
}

ArchModule::~ArchModule()
{
}

const char*
ArchModule::getType() const
{
    return "Arch";
}

} // namespace yasm

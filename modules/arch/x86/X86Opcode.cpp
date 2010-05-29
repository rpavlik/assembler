//
// x86 core bytecode
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "X86Opcode.h"

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/Bytes.h"


using namespace yasm;
using namespace yasm::arch;

YAML::Emitter&
arch::operator<< (YAML::Emitter& out, const X86Opcode& opcode)
{
    if (opcode.m_len == 0)
    {
        out << YAML::Null;
        return out;
    }

    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "opcode" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    out << YAML::Hex << static_cast<unsigned int>(opcode.m_opcode[0]);
    out << YAML::Hex << static_cast<unsigned int>(opcode.m_opcode[1]);
    out << YAML::Hex << static_cast<unsigned int>(opcode.m_opcode[2]);
    out << YAML::EndSeq;

    out << YAML::Key << "length";
    out << YAML::Value << static_cast<unsigned int>(opcode.m_len);
    out << YAML::EndMap;
    return out;
}

void
X86Opcode::ToBytes(Bytes& bytes) const
{
    bytes.Write(m_opcode, m_len);
}

void
X86Opcode::MakeAlt1()
{
    m_opcode[0] = m_opcode[m_len];
    m_len = 1;
}

void
X86Opcode::MakeAlt2()
{
    m_opcode[0] = m_opcode[1];
    m_opcode[1] = m_opcode[2];
    m_len = 2;
}

#include "ExecInput/ExecInput.hpp"

#include "Utils/Common.hpp"

u64 ExecInput::id_counter = 0;

ExecInput::ExecInput()
    : id(id_counter++), 
      len(0) {}

ExecInput::~ExecInput() {}

ExecInput::ExecInput(const u8* orig_buf, u32 len) 
    : id(id_counter++),
      buf(new u8[len]),
      len(len)
{
    std::memcpy(buf.get(), orig_buf, len);
}

ExecInput::ExecInput(std::unique_ptr<u8[]>&& orig_buf, u32 len) 
    : id(id_counter++),
      buf(orig_buf.release()),
      len(len) {}

ExecInput::ExecInput(ExecInput&& orig)
    : id(orig.id),
      buf(std::move(orig.buf)),
      len(orig.len) {}

ExecInput& ExecInput::operator=(ExecInput&& orig) {
    id = orig.id;
    buf = std::move(orig.buf);
    len = orig.len;
    return *this;
}

u8* ExecInput::GetBuf() const {
    return buf.get();
}

u32 ExecInput::GetLen() const {
    return len;
}

u64 ExecInput::GetID() const {
    return id;
}

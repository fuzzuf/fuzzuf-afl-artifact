#include "ExecInput/OnMemoryExecInput.hpp"

#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"

class ExecInput;

OnMemoryExecInput::OnMemoryExecInput(const u8* orig_buf, u32 len)
    : ExecInput(orig_buf, len) {}

OnMemoryExecInput::OnMemoryExecInput(std::unique_ptr<u8[]>&& orig_buf, u32 len)
    : ExecInput(std::move(orig_buf), len) {}

OnMemoryExecInput::OnMemoryExecInput(OnMemoryExecInput&& orig)
    : ExecInput(std::move(orig)) {}

OnMemoryExecInput& OnMemoryExecInput::operator=(OnMemoryExecInput&& orig) {
    ExecInput::operator=(std::move(orig));
    return *this;
}

void OnMemoryExecInput::LoadIfNotLoaded(void) {}
void OnMemoryExecInput::Load(void) {}
void OnMemoryExecInput::Unload(void) {}
void OnMemoryExecInput::Save(void) {}

void OnMemoryExecInput::OverwriteKeepingLoaded(const u8* new_buf, u32 new_len) {
    buf.reset(new u8 [new_len]);
    len = new_len;
    std::memcpy(buf.get(), new_buf, len);
}

void OnMemoryExecInput::OverwriteKeepingLoaded(
    std::unique_ptr<u8[]>&& new_buf, u32 new_len
) {
    buf.reset(new_buf.release());
    len = new_len;
}

void OnMemoryExecInput::OverwriteThenUnload(const u8* new_buf, u32 new_len) {
    OverwriteKeepingLoaded(new_buf, new_len); // same
}

void OnMemoryExecInput::OverwriteThenUnload(
    std::unique_ptr<u8[]>&& new_buf, u32 new_len
) {
    OverwriteKeepingLoaded(std::move(new_buf), new_len); // same
}

void OnMemoryExecInput::SaveToFile(const fs::path& path) {
    int fd = Util::OpenFile(path.string(), O_WRONLY | O_CREAT, 0600);
    Util::WriteFile(fd, buf.get(), len);
    Util::CloseFile(fd);
}

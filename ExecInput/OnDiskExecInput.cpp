#include "ExecInput/OnDiskExecInput.hpp"

#include "Utils/Common.hpp"
#include "Utils/Filesystem.hpp"
#include "ExecInput/ExecInput.hpp"
#include "Logger/Logger.hpp"

class ExecInput;

OnDiskExecInput::OnDiskExecInput(const fs::path& path, bool hardlinked) 
    : ExecInput(),
      path(path),
      hardlinked(hardlinked) {}

OnDiskExecInput::OnDiskExecInput(OnDiskExecInput&& orig)
    : ExecInput(std::move(orig)),
      path(std::move(orig.path)),
      hardlinked(orig.hardlinked) {}

OnDiskExecInput& OnDiskExecInput::operator=(OnDiskExecInput&& orig) {
    ExecInput::operator=(std::move(orig));
    path = std::move(orig.path);
    hardlinked = orig.hardlinked;
    return *this;
}

void OnDiskExecInput::ReallocBufIfLack(u32 new_len) {
    if (!buf || new_len > len) {
        buf.reset(new u8[new_len], []( u8 *p ){ if( p ) delete [] p; });
    }
    len = new_len;
}

void OnDiskExecInput::LoadIfNotLoaded(void) {
    if (buf) return;
    Load();
}

void OnDiskExecInput::Load(void) {
    ReallocBufIfLack(fs::file_size(path));

    int fd = Util::OpenFile(path.string(), O_RDONLY);
    Util::ReadFile(fd, buf.get(), len);
    Util::CloseFile(fd);
}

void OnDiskExecInput::Unload(void) {
    buf.reset();
}

void OnDiskExecInput::Save(void) {
    if (hardlinked) {
        Util::DeleteFileOrDirectory(path.string());
        hardlinked = false;
    }

    int fd = Util::OpenFile(path.string(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    Util::WriteFile(fd, buf.get(), len);
    Util::CloseFile(fd);
}

void OnDiskExecInput::OverwriteKeepingLoaded(const u8* new_buf, u32 new_len) {
    ReallocBufIfLack(new_len);
    std::memcpy(buf.get(), new_buf, len);
    Save();
}

void OnDiskExecInput::OverwriteKeepingLoaded(
    std::unique_ptr<u8[]>&& new_buf, u32 new_len
) {
    buf.reset(new_buf.release());
    
    len = new_len;
    Save();
}

void OnDiskExecInput::OverwriteThenUnload(const u8* new_buf, u32 new_len) {
    buf.reset();

    int fd = Util::OpenFile(path.string(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    Util::WriteFile(fd, new_buf, new_len);
    Util::CloseFile(fd);
}

void OnDiskExecInput::OverwriteThenUnload(
    std::unique_ptr<u8[]>&& new_buf, u32 new_len
) {
    auto will_delete = std::move(new_buf);
    OverwriteThenUnload(will_delete.get(), new_len);
}

void OnDiskExecInput::LoadByMmap(void) {
    int fd = Util::OpenFile( path.string(), O_RDONLY );
    auto file_len = fs::file_size(path);

    auto raw_buf = static_cast<u8*>(
        mmap( nullptr, file_len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)
    );

    Util::CloseFile(fd);

    if (raw_buf == MAP_FAILED) {
        ERROR("Unable to mmap '%s' : %s", path.c_str(), strerror( errno ));
	  }
      
	  buf.reset( raw_buf, [file_len]( u8 *p ){ if( p ) munmap( p, file_len ); } );
    len = file_len;
}

bool OnDiskExecInput::Link(const fs::path& dest_path) {
    return link(path.c_str(), dest_path.c_str()) == 0;
}

void OnDiskExecInput::Copy(const fs::path& dest_path) {
    Util::CopyFile(path.string(), dest_path.string());
}

bool OnDiskExecInput::LinkAndRefer(const fs::path& new_path) {
    if (!Link(new_path)) return false;
    path = new_path;
    hardlinked = true;
    return true;
}

void OnDiskExecInput::CopyAndRefer(const fs::path& new_path) {
    // don't want to overwrite the content if new_path is a hardlink
    Util::DeleteFileOrDirectory(new_path.string());
    Copy(new_path);
    path = new_path;
    hardlinked = false;
}

const fs::path& OnDiskExecInput::GetPath(void) const {
    return path;
}

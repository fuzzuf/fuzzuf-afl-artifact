#include "Utils/Common.hpp"
#include "Logger/Logger.hpp"

void SetupDirs(std::string out_dir) {
    try {
        DEBUG("SetupDir\n");
        Util::CreateDir(out_dir);
        Util::CreateDir(out_dir + "/queue");
        Util::CreateDir(out_dir + "/queue/.state/");
        Util::CreateDir(out_dir + "/queue/.state/deterministic_done/");
        Util::CreateDir(out_dir + "/queue/.state/auto_extras/");
        Util::CreateDir(out_dir + "/queue/.state/redundant_edges/");
        Util::CreateDir(out_dir + "/queue/.state/variable_behavior/");
        Util::CreateDir(out_dir + "/crashes");
        Util::CreateDir(out_dir + "/hangs");
        
    } catch( const FileError &e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

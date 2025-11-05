#ifndef FILE_INFO_HPP
#define FILE_INFO_HPP

class FileInfo {
private:
    std::string m_path;
    long long m_size;
    std::string m_hash;
    bool m_isDir;
    bool m_isDuplicate = false;
    bool m_isParent = false;

    bool isExecutable() const {
        if (m_isDir) return false;
        
        namespace fs = std::filesystem;
        try {
            auto perms = fs::status(m_path).permissions();
            return (perms & fs::perms::owner_exec) != fs::perms::none;
        } catch (...) {
            return false;
        }
    }


public:
    FileInfo(const std::string& p, long long s, bool isDir, bool isParent = false)
        : m_path(p), m_size(s), m_hash(""), m_isDir(isDir), m_isParent(isParent) {}

    // Bestehende Getter...
    const std::string& getPath() const { return m_path; }
    long long getFileSize() const { return m_size; }
    const std::string& getHash() const { return m_hash; }
    bool isDirectory() const { return m_isDir; }
    
   std::string getDisplayName() const {
        if (m_isParent) return "..";  // <-- Parent Dir
        
        std::filesystem::path p(m_path);
        std::string name = p.filename().string();
        
        if (name.empty() && m_isDir) {
            // Root oder current dir
            return p.string();
        }
        
        return m_isDir ? name + "/" : name;
    }
    
    bool isParentDir() const { return m_isParent; }

     // NEU: Farbe basierend auf Dateityp/Status
    int getColorCode() const {
        if (m_size == 0 && !m_isDir) return 1;      // Rot: 0-Byte Files
        if (m_isDuplicate) return 3;                 // Gelb: Duplikate
        if (m_isDir) return 4;                       // Blau: Directories
        if (isExecutable()) return 2;                // Grün: Executables
        return 7;                                    // Weiß: Normal
    }

    
    std::string getSizeFormatted() const {
        if (m_isDir) return "<DIR>";
        if (m_size == 0) return "0 B";
        
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(m_size);
        
        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f %s", size, units[unit]);
        return std::string(buf);
    }
    
    // NEU: Für Duplikat-Erkennung
    bool isDuplicate() const { return m_isDuplicate; }
    void setDuplicate(bool dup) { m_isDuplicate = dup; }
    
    void setHash(const std::string& hash) { m_hash = hash; }

    bool zeroFiles() const {
        return (m_size == 0 && !m_isDir);
    }
};

#endif // FILE_INFO_HPP

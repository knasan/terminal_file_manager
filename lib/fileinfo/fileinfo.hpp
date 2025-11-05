#ifndef FILE_INFO_HPP
#define FILE_INFO_HPP

class FileInfo {
private:
    std::string m_path;
    long long m_size;
    std::string m_hash;
    bool m_isDir;
    bool m_isDuplicate = false;  // NEU: für UI-Highlighting

public:
    FileInfo(const std::string& p, long long s, bool isDir)
    : m_path(p), m_size(s), m_hash(""), m_isDir(isDir) {}

    // Bestehende Getter...
    const std::string& getPath() const { return m_path; }
    long long getFileSize() const { return m_size; }
    const std::string& getHash() const { return m_hash; }
    bool isDirectory() const { return m_isDir; }
    
    // NEU: Für FTXUI
    std::string getDisplayName() const {
        std::filesystem::path p(m_path);
        std::string name = p.filename().string();
        return m_isDir ? name + "/" : name;
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

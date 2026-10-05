// =============================================================================
//  stats.hpp  -  timing, memory and CSV module
// -----------------------------------------------------------------------------
//  HOW TO ADD A NEW METRIC (same idea as the Lab 1 Battleship CSV module):
//    1. compute the value (in driver.hpp, function  fillRecord(), or let a solver
//       push it into  SolveOut::extra  with  out.extra.push_back({"name", value}) ),
//    2. call  rec.add("column_name", value).
//  The CSV header is built automatically from the first record, so nothing else
//  has to be changed.
// =============================================================================
#pragma once
#include <sys/resource.h>
#include <cstring>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rc {

using Clock = std::chrono::steady_clock;
inline double secondsSince(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

// ------------------------------------------------------------------ memory (Linux)
inline double readProcStatusMB(const char* key) {
    std::FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    double mb = -1;
    size_t n = std::string(key).size();
    while (std::fgets(line, sizeof line, f)) {
        if (std::strncmp(line, key, n) == 0) {
            long kb = 0;
            std::sscanf(line + n, " %ld", &kb);
            mb = kb / 1024.0;
            break;
        }
    }
    std::fclose(f);
    return mb;
}
inline double currentRssMB() { return readProcStatusMB("VmRSS:"); }
inline double peakRssMB() {
    double v = readProcStatusMB("VmHWM:");
    if (v >= 0) return v;
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss / 1024.0;  // Linux: kilobytes
}
// Resets the "peak resident memory" counter so every test is measured on its own.
inline bool resetPeakRss() {
    std::FILE* f = std::fopen("/proc/self/clear_refs", "w");
    if (!f) return false;
    bool ok = std::fputs("5", f) >= 0;
    ok = (std::fclose(f) == 0) && ok;
    return ok;
}

// ------------------------------------------------------------------ one CSV row
class Record {
public:
    Record& add(const std::string& k, const std::string& v) { cols_.emplace_back(k, v); return *this; }
    Record& add(const std::string& k, const char* v) { return add(k, std::string(v)); }
    Record& add(const std::string& k, double v, int prec = 6) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.*f", prec, v);
        return add(k, std::string(buf));
    }
    Record& add(const std::string& k, int v) { return add(k, std::to_string(v)); }
    Record& add(const std::string& k, long long v) { return add(k, std::to_string(v)); }
    Record& add(const std::string& k, unsigned long long v) { return add(k, std::to_string(v)); }
    Record& add(const std::string& k, unsigned long v) { return add(k, std::to_string((unsigned long long)v)); }
    Record& add(const std::string& k, bool v) { return add(k, std::string(v ? "1" : "0")); }
    const std::vector<std::pair<std::string, std::string>>& cols() const { return cols_; }

private:
    std::vector<std::pair<std::string, std::string>> cols_;
};

// ------------------------------------------------------------------ CSV file (append mode)
class CsvWriter {
public:
    bool open(const std::string& path) {
        path_ = path;
        std::ifstream in(path);
        if (in && std::getline(in, existingHeader_)) headerWritten_ = true;  // file already has a header
        in.close();
        out_.open(path, std::ios::app);
        return (bool)out_;
    }
    bool isOpen() const { return (bool)out_ && !path_.empty(); }
    // returns false (and writes nothing) if the columns differ from the ones already in the file
    bool write(const Record& r) {
        if (!isOpen()) return false;
        std::string header;
        for (size_t i = 0; i < r.cols().size(); i++) { if (i) header += ','; header += r.cols()[i].first; }
        if (!headerWritten_) { out_ << header << "\n"; headerWritten_ = true; existingHeader_ = header; }
        else if (header != existingHeader_) {
            std::fprintf(stderr, "CSV: columns differ from the header of %s. Use another file name.\n", path_.c_str());
            return false;
        }
        for (size_t i = 0; i < r.cols().size(); i++) {
            if (i) out_ << ',';
            out_ << escape(r.cols()[i].second);
        }
        out_ << "\n";
        out_.flush();
        return true;
    }

private:
    static std::string escape(const std::string& s) {
        if (s.find_first_of(",\"\n") == std::string::npos) return s;
        std::string o = "\"";
        for (char c : s) { if (c == '"') o += '"'; o += c; }
        return o + "\"";
    }
    std::string path_, existingHeader_;
    std::ofstream out_;
    bool headerWritten_ = false;
};

}  // namespace rc

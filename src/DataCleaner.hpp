#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::steady_clock;
using ns = std::chrono::nanoseconds;

// CSV helper
void rstrip_cr(std::string& s);
void split_comma_sv(const std::string& line, std::vector<std::string_view>& out);

// Pipeline config
struct PipelineConfig {
    std::string inputPath;
    std::string outputCleanPath;
    std::string outputDroppedPath;

    std::vector<std::string> keepColumns;
    std::string gesturePresenceCol;
    std::string frameNumCol;

    std::vector<std::string> excludeFromClean;

    bool printDroppedToStderr = false;
    size_t readerBufferBytes = (1u << 16);  // 64 KB
};

class CsvReader {
public:
    CsvReader(const std::string& path, size_t bufferBytes);
    bool open();
    bool readHeader(std::vector<std::string>& headersOut,
                    std::unordered_map<std::string, int>& nameToIndexOut);
    bool readLine(std::string& lineOut);
    void close();

private:
    std::string inputPath_;
    size_t bufferBytes_;
    std::ifstream fin_;
    std::string headerLine_;
    std::vector<char> buffer_;
};

class CsvWriter {
public:
    CsvWriter(const std::string& path);
    bool open();
    void writeHeader(const std::vector<std::string>& names);
    void writeHeaderSubset(const std::vector<std::string>& names, const std::vector<int>& positions);
    void writeRowFull(const std::vector<std::string_view>& projected);
    void writeRowSubset(const std::vector<std::string_view>& projected, const std::vector<int>& positions);
    void close();

private:
    std::string outputPath_;
    std::ofstream fout_;
};

class ColumnProjector {
public:
    ColumnProjector(std::vector<std::string> keepColumns,
                    const std::unordered_map<std::string, int>& nameToIndex);
    void project(const std::vector<std::string_view>& rawCells,
                 std::vector<std::string_view>& outProjected) const;
    const std::vector<std::string>& keepNames() const;
    std::vector<int> positionsExcluding(const std::vector<std::string>& toExclude) const;
    size_t missingKeptCount() const;
    size_t removedColumnsApprox(size_t inputColumnCount) const;

private:
    std::vector<std::string> keepNames_;
    std::vector<int> keepIndices_;
    size_t missingKept_;
};

class RecordFilter {
public:
    virtual ~RecordFilter() = default;
    virtual bool shouldDrop(const std::vector<std::string_view>& rawCells,
                            std::string& reasonOut) const = 0;
};

class GesturePresenceZeroFilter : public RecordFilter {
public:
    explicit GesturePresenceZeroFilter(int idx);
    bool shouldDrop(const std::vector<std::string_view>& rawCells,
                    std::string& reasonOut) const override;

private:
    int idx_;
};

class FrameNumEmptyFilter : public RecordFilter {
public:
    explicit FrameNumEmptyFilter(int idx);
    bool shouldDrop(const std::vector<std::string_view>& rawCells,
                    std::string& reasonOut) const override;

private:
    int idx_;
};

class CompositeFilter {
public:
    void add(std::unique_ptr<RecordFilter> filter);
    bool shouldDrop(const std::vector<std::string_view>& rawCells, std::string& reasonOut) const;

private:
    std::vector<std::unique_ptr<RecordFilter>> filters_;
};

class Bench {
public:
    void addSplit(ns d);
    void addProject(ns d);
    void addFilter(ns d);
    void addWriteClean(ns d);
    void addWriteDrop(ns d);
    void setTotal(ns d);
    void printSummary(size_t total, size_t kept, size_t dropped) const;

private:
    ns durSplit_{0};
    ns durProject_{0};
    ns durFilter_{0};
    ns durWriteClean_{0};
    ns durWriteDrop_{0};
    ns durTotal_{0};
};

class DataCleaningPipeline {
public:
    explicit DataCleaningPipeline(PipelineConfig cfg);
    int run();

private:
    static int indexOf(const std::unordered_map<std::string, int>& map, const std::string& key);
    PipelineConfig cfg_;
    Bench bench_;
};

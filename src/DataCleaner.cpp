#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataCleaner.hpp"

// Console colors
#define COLOR_RESET "\033[0m"
#define COLOR_INFO "\033[36m"
#define COLOR_STAGE "\033[37m"
#define COLOR_DROP "\033[31m"
#define COLOR_SUMMARY "\033[33m"
#define COLOR_BENCH "\033[32m"

static inline double to_ms(const ns& d) {
    return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(d).count();
}

// Strip trailing '\r'
void rstrip_cr(std::string& s) {
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

void split_comma_sv(const std::string& line, std::vector<std::string_view>& out) {
    out.clear();

    const char* s = line.data();
    
    size_t n = line.size();
    size_t start = 0;
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == ',') {
            out.emplace_back(s + start, i - start);
            start = i + 1;
        }
    }

    out.emplace_back(s + start, n - start);
}

// CsvReader implementation
CsvReader::CsvReader(const std::string& path, size_t bufferBytes)
    : inputPath_(path), bufferBytes_(bufferBytes) {}

bool CsvReader::open() {
    fin_.open(inputPath_, std::ios::in | std::ios::binary);

    if (!fin_) {
        return false;
    }
        
    if (bufferBytes_ > 0) {
        buffer_.resize(bufferBytes_);
        fin_.rdbuf()->pubsetbuf(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
    }

    return true;
}

bool CsvReader::readHeader(std::vector<std::string>& headersOut,
                           std::unordered_map<std::string, int>& nameToIndexOut) {
    headersOut.clear();
    nameToIndexOut.clear();

    if (!std::getline(fin_, headerLine_)) {
        return false;
    }

    rstrip_cr(headerLine_);

    std::vector<std::string_view> sv;
    split_comma_sv(headerLine_, sv);
    headersOut.reserve(sv.size());
    
    for (int i = 0; i < static_cast<int>(sv.size()); ++i) {
        headersOut.emplace_back(sv[i]);
        nameToIndexOut.emplace(headersOut.back(), i);
    }

    return true;
}

bool CsvReader::readLine(std::string& lineOut) {
    if (!std::getline(fin_, lineOut)) {
        return false;
    }
    rstrip_cr(lineOut);
    return true;
}

void CsvReader::close() {
    if (fin_.is_open()) {
        fin_.close();
    }
}

// CsvWriter implementation
CsvWriter::CsvWriter(const std::string& path) : outputPath_(path) {}

bool CsvWriter::open() {
    fout_.open(outputPath_, std::ios::out | std::ios::binary);
    return static_cast<bool>(fout_);
}

void CsvWriter::writeHeader(const std::vector<std::string>& names) {
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) {
            fout_ << ",";
        }
        fout_ << names[i];
    }
    fout_ << "\n";
}

void CsvWriter::writeHeaderSubset(const std::vector<std::string>& names, const std::vector<int>& positions) {
    bool first = true;
    for (int pos : positions) {
        if (!first) {
            fout_ << ",";
        }
        fout_ << names[static_cast<size_t>(pos)];
        first = false;
    }
    fout_ << "\n";
}

void CsvWriter::writeRowFull(const std::vector<std::string_view>& projected) {
    for (size_t i = 0; i < projected.size(); ++i) {
        if (i) {
            fout_ << ",";
        }
        fout_.write(projected[i].data(), static_cast<std::streamsize>(projected[i].size()));
    }
    fout_ << "\n";
}

void CsvWriter::writeRowSubset(const std::vector<std::string_view>& projected, const std::vector<int>& positions) {
    bool first = true;
    for (int pos : positions) {
        if (!first) {
            fout_ << ",";
        }
        const auto& cell = projected[static_cast<size_t>(pos)];
        fout_.write(cell.data(), static_cast<std::streamsize>(cell.size()));
        first = false;
    }
    fout_ << "\n";
}

void CsvWriter::close() {
    if (fout_.is_open()) {
        fout_.close();
    }
}

// ColumnProjector implementation
ColumnProjector::ColumnProjector(std::vector<std::string> keepColumns,
                                 const std::unordered_map<std::string, int>& nameToIndex)
    : keepNames_(std::move(keepColumns)) {
    keepIndices_.clear();
    keepIndices_.reserve(keepNames_.size());
    missingKept_ = 0;
    
    for (const auto& name : keepNames_) {
        auto it = nameToIndex.find(name);
        if (it == nameToIndex.end()) {
            keepIndices_.push_back(-1);
            ++missingKept_;
        } else {
            keepIndices_.push_back(it->second);
        }
    }
}

void ColumnProjector::project(const std::vector<std::string_view>& rawCells,
                              std::vector<std::string_view>& outProjected) const {
    outProjected.clear();
    outProjected.reserve(keepIndices_.size());
    for (int k : keepIndices_) {
        if (k >= 0 && k < static_cast<int>(rawCells.size())) {
            outProjected.emplace_back(rawCells[static_cast<size_t>(k)]);
        } else {
            outProjected.emplace_back("", 0);
        }
    }
}

const std::vector<std::string>& ColumnProjector::keepNames() const {
    return keepNames_;
}

std::vector<int> ColumnProjector::positionsExcluding(const std::vector<std::string>& toExclude) const {
    std::vector<int> pos;
    pos.reserve(keepIndices_.size());
    for (int i = 0; i < static_cast<int>(keepNames_.size()); ++i) {
        bool excluded = false;
        for (const auto& name : toExclude) {
            if (keepNames_[static_cast<size_t>(i)] == name) {
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            pos.push_back(i);
        }
    }
    return pos;
}

size_t ColumnProjector::missingKeptCount() const {
    return missingKept_;
}

size_t ColumnProjector::removedColumnsApprox(size_t inputColumnCount) const {
    return (inputColumnCount >= keepNames_.size())
               ? (inputColumnCount - keepNames_.size())
               : 0;
}

// Record filters implementation
GesturePresenceZeroFilter::GesturePresenceZeroFilter(int idx) : idx_(idx) {}

bool GesturePresenceZeroFilter::shouldDrop(const std::vector<std::string_view>& rawCells,
                                           std::string& reasonOut) const {
    if (idx_ < 0) {
        return false; 
    }
    if (idx_ >= static_cast<int>(rawCells.size())) {
        return false;
    }
    if (rawCells[static_cast<size_t>(idx_)] == "0") {
        reasonOut = "gesturePresence = 0";
        return true;
    }
    return false;
}

FrameNumEmptyFilter::FrameNumEmptyFilter(int idx) : idx_(idx) {}

bool FrameNumEmptyFilter::shouldDrop(const std::vector<std::string_view>& rawCells,
                                     std::string& reasonOut) const {
    if (idx_ < 0) {
        return false;
    }
    if (idx_ >= static_cast<int>(rawCells.size())) {
        reasonOut = "frameNum missing   ";
        return true;
    }
    if (rawCells[static_cast<size_t>(idx_)].empty()) {
        reasonOut = "frameNum empty     ";
        return true;
    }
    return false;
}

void CompositeFilter::add(std::unique_ptr<RecordFilter> filter) {
    filters_.emplace_back(std::move(filter));
}

bool CompositeFilter::shouldDrop(const std::vector<std::string_view>& rawCells, std::string& reasonOut) const {
    for (const auto& f : filters_) {
        if (f->shouldDrop(rawCells, reasonOut)) {
            return true;
        }
    }
    return false;
}

// Benchmarking implementation
void Bench::addSplit(ns d) { 
    durSplit_ += d; 
}

void Bench::addProject(ns d) { 
    durProject_ += d; 
}

void Bench::addFilter(ns d) { 
    durFilter_ += d; 
}

void Bench::addWriteClean(ns d) { 
    durWriteClean_ += d; 
}

void Bench::addWriteDrop(ns d) { 
    durWriteDrop_ += d; 
}

void Bench::setTotal(ns d) { 
    durTotal_ = d; 
}

void Bench::printSummary(size_t total, size_t kept, size_t dropped) const {
    std::cerr << COLOR_SUMMARY "\n[SUMMARY]" COLOR_RESET
              << " Total rows   = " << std::setw(7) << total
              << "\n          Rows kept    = " << std::setw(7) << kept
              << "\n          Rows dropped = " << std::setw(7) << dropped << "\n";

    const double msAfterColumn = to_ms(durProject_);
    const double msAfterRecord = to_ms(durProject_ + durFilter_);
    const double msTotal = to_ms(durTotal_);

    std::cerr << COLOR_BENCH "\n[BENCH]" COLOR_RESET
              << " Column filtering: " << std::setw(10) << msAfterColumn << " ms\n";
    std::cerr << COLOR_BENCH "[BENCH]" COLOR_RESET
              << " Record filtering: " << std::setw(10) << msAfterRecord << " ms\n";
    std::cerr << COLOR_BENCH "[BENCH]" COLOR_RESET
              << " Total time:       " << std::setw(10) << msTotal << " ms\n";
}

// DataCleaningPipeline implementation
DataCleaningPipeline::DataCleaningPipeline(PipelineConfig cfg) : cfg_(std::move(cfg)) {}

int DataCleaningPipeline::run() {
    std::cerr << COLOR_INFO "\n[INFO] " COLOR_RESET "Starting data cleaning pipeline...\n";
    std::cerr << COLOR_INFO "\n[INFO] " COLOR_RESET "Input         = " << cfg_.inputPath
              << "\n       OutputClean   = " << cfg_.outputCleanPath
              << "\n       OutputDropped = " << cfg_.outputDroppedPath << "\n";

    const auto tStart = Clock::now();

    // Reader
    CsvReader reader(cfg_.inputPath, cfg_.readerBufferBytes);
    if (!reader.open()) {
        std::cerr << "ERROR: cannot open input: " << cfg_.inputPath << "\n";
        return 1;
    }

    // Header
    std::vector<std::string> headerNames;
    std::unordered_map<std::string, int> nameToIndex;
    std::cerr << COLOR_STAGE "\n[STAGE 0] " COLOR_RESET "Schema mapping: reading header and building index...\n";
    if (!reader.readHeader(headerNames, nameToIndex)) {
        std::cerr << "ERROR: empty file or failed to read header\n";
        return 1;
    }
    std::cerr << COLOR_STAGE "\n[STAGE 0] " COLOR_RESET "Input columns = " << headerNames.size() << "\n";

    // Projection
    ColumnProjector projector(cfg_.keepColumns, nameToIndex);
    std::cerr << COLOR_STAGE "\n[STAGE 1] " COLOR_RESET "Column pruning: removing columns and projecting... "
              << "(kept = " << projector.keepNames().size()
              << ", removed = " << projector.removedColumnsApprox(headerNames.size())
              << ", missing in input = " << projector.missingKeptCount() << ")\n";

    // Output subset for CLEAN file
    const auto cleanPositions = projector.positionsExcluding(cfg_.excludeFromClean);

    // Filter setup
    const int idxGesturePresence = indexOf(nameToIndex, cfg_.gesturePresenceCol);
    const int idxFrameNum = indexOf(nameToIndex, cfg_.frameNumCol);
    CompositeFilter filter;
    filter.add(std::make_unique<GesturePresenceZeroFilter>(idxGesturePresence));
    filter.add(std::make_unique<FrameNumEmptyFilter>(idxFrameNum));
    std::cerr << COLOR_STAGE "\n[STAGE 2] " COLOR_RESET "Record filtering: cleaning data...\n\n";

    // Writers
    CsvWriter cleanWriter(cfg_.outputCleanPath);
    if (!cleanWriter.open()) {
        std::cerr << "ERROR: cannot open output: " << cfg_.outputCleanPath << "\n";
        return 1;
    }
    CsvWriter droppedWriter(cfg_.outputDroppedPath);
    if (!droppedWriter.open()) {
        std::cerr << "ERROR: cannot open output: " << cfg_.outputDroppedPath << "\n";
        return 1;
    }

    // Headers
    cleanWriter.writeHeaderSubset(projector.keepNames(), cleanPositions);
    droppedWriter.writeHeader(projector.keepNames());

    // Process rows
    size_t rowsTotal = 0, rowsKept = 0, rowsDropped = 0;
    std::string line;
    std::vector<std::string_view> rawCells;
    rawCells.reserve(headerNames.size() + 8);

    std::vector<std::string_view> projected;
    projected.reserve(projector.keepNames().size());

    for (;;) {
        if (!reader.readLine(line)) {
            break;
        }
        
        ++rowsTotal;

        const auto t0 = Clock::now();
        split_comma_sv(line, rawCells);
        const auto t1 = Clock::now();
        bench_.addSplit(t1 - t0);

        // Project
        projector.project(rawCells, projected);
        const auto t2 = Clock::now();
        bench_.addProject(t2 - t1);

        // Filter
        std::string reason;
        const bool drop = filter.shouldDrop(rawCells, reason);
        const auto t3 = Clock::now();
        bench_.addFilter(t3 - t2);

        if (drop) {
            droppedWriter.writeRowFull(projected);
            const auto t4 = Clock::now();
            bench_.addWriteDrop(t4 - t3);
            ++rowsDropped;

            if (cfg_.printDroppedToStderr) {
                std::cerr << COLOR_DROP "[DROP] " COLOR_RESET "reason: " << reason << std::setw(8) << " row = ";
                for (size_t i = 0; i < projected.size(); ++i) {
                    if (i) {
                        std::cerr << ", ";
                    }
                    std::cerr.write(projected[i].data(), static_cast<std::streamsize>(projected[i].size()));
                }
                std::cerr << "\n";
            }
        } else {
            cleanWriter.writeRowSubset(projected, cleanPositions);
            const auto t4 = Clock::now();
            bench_.addWriteClean(t4 - t3);
            ++rowsKept;
        }
    }

    reader.close();
    cleanWriter.close();
    droppedWriter.close();

    const auto tEnd = Clock::now();
    bench_.setTotal(tEnd - tStart);

    std::cerr << COLOR_STAGE "\n[STAGE 3] " COLOR_RESET "Materialization: wrote outputs\n";
    std::cerr << "    - Cleaned rows: " << std::setw(6) << rowsKept << " --> " << cfg_.outputCleanPath << "\n";
    std::cerr << "    - Dropped rows: " << std::setw(6) << rowsDropped << " --> " << cfg_.outputDroppedPath << "\n";

    bench_.printSummary(rowsTotal, rowsKept, rowsDropped);
    return 0;
}

int DataCleaningPipeline::indexOf(const std::unordered_map<std::string, int>& map, const std::string& key) {
    auto it = map.find(key);
    return (it == map.end()) ? -1 : it->second;
}

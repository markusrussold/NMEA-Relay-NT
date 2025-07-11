#include "pch.h"
#include "ReportQueue.h"
#include <regex>
#include <filesystem>

//#define DEBUGMODE

namespace fs = std::filesystem;

// Constructor that accepts a folder path
ReportQueue::ReportQueue(const std::string& path)
    : folder_path(path), file_index(0), entry_count(0), total_entry_count(0), total_size_in_bytes(0) {

    // Ensure the directory exists
    fs::create_directories(folder_path);

    load_existing_files();

    if (file_stack.empty()) {
        // No files found, start fresh
        file_index = 0;
        filename = generate_new_filename();
        file_stack.push_back(filename);
    }
    else {
        file_index = file_stack.size();
        filename = file_stack.back();
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs) {
            size_t length;
            while (ifs.read(reinterpret_cast<char*>(&length), sizeof(length))) {
                ifs.seekg(length, std::ios::cur);
                entry_count++;
            }
        }
    }
}

// Function to extract the numeric index from the filename
int extract_index(const std::string& filename) {
    std::regex re("report_queue_(\\d+)\\.dat");
    std::smatch match;
    if (std::regex_search(filename, match, re) && match.size() > 1) {
        return std::stoi(match.str(1));
    }
    return -1; // Return -1 if the pattern doesn't match (shouldn't happen with valid filenames)
}

void ReportQueue::load_existing_files() {
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        const auto& path = entry.path();
        const auto& filename = path.filename().string();

        if (filename.find("report_queue_") == 0 && filename.find(".dat") != std::string::npos) {
            file_stack.push_back(path.string());

            std::ifstream ifs(path.string(), std::ios::binary);
            if (ifs) {
                size_t length;
                while (ifs.read(reinterpret_cast<char*>(&length), sizeof(length))) {
                    ifs.seekg(length, std::ios::cur);
                    total_entry_count++;
                    total_size_in_bytes += sizeof(length) + length;
                }
            }
        }
    }

    std::sort(file_stack.begin(), file_stack.end(), [](const std::string& a, const std::string& b) {
        return extract_index(a) < extract_index(b);
        });
}

void ReportQueue::push(const std::string& sentence) {
    std::lock_guard<std::mutex> lock(mutex);

    if (file_stack.empty() || entry_count >= MAX_ENTRIES_PER_FILE) {
        filename = generate_new_filename();
        file_stack.push_back(filename);
        entry_count = 0;
    }

    std::ofstream ofs(filename, std::ios::binary | std::ios::app);
    if (!ofs) {
        std::cerr << "Failed to open file for appending: " << filename << std::endl;
        return;
    }

    size_t length = sentence.size();
    ofs.write(reinterpret_cast<const char*>(&length), sizeof(length));
    ofs.write(sentence.data(), length);

    entry_count++;
    total_entry_count++;
    total_size_in_bytes += sizeof(length) + length;
}

bool ReportQueue::try_pop(std::string& sentence) {
    std::lock_guard<std::mutex> lock(mutex);
    sentence.clear();

    while (!file_stack.empty()) {
        const std::string& current_file = file_stack.front();
        std::ifstream ifs(current_file, std::ios::binary);
        if (!ifs) return false;

        std::vector<std::pair<size_t, std::string>> records;
        size_t length;
        while (ifs.read(reinterpret_cast<char*>(&length), sizeof(length))) {
            std::string record(length, '\0');
            ifs.read(&record[0], length);
            records.emplace_back(length, std::move(record));
        }
        ifs.close();

        if (records.empty()) {
            std::remove(current_file.c_str());
            file_stack.erase(file_stack.begin());
            continue;
        }

        sentence = std::move(records.front().second);

        if (records.size() == 1) {
            std::remove(current_file.c_str());
            file_stack.erase(file_stack.begin());
        }
        else {
            std::ofstream ofs(current_file, std::ios::binary | std::ios::trunc);
            for (size_t i = 1; i < records.size(); ++i) {
                ofs.write(reinterpret_cast<const char*>(&records[i].first), sizeof(records[i].first));
                ofs.write(records[i].second.data(), records[i].first);
            }
        }

        entry_count = total_entry_count % MAX_ENTRIES_PER_FILE;
        total_entry_count--;
        total_size_in_bytes -= (sizeof(length) + sentence.size());

        return true;
    }

    return false;
}

bool ReportQueue::empty() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return total_entry_count == 0;
}

int ReportQueue::size() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return total_entry_count;
}

int ReportQueue::size_in_bytes() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return total_size_in_bytes;
}

std::string ReportQueue::generate_new_filename() {
    std::stringstream ss;
    ss << folder_path << "/report_queue_" << ++file_index << ".dat";
    return ss.str();
}
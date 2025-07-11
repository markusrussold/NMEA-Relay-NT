#ifndef REPORTQUEUE_H
#define REPORTQUEUE_H

#include "pch.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>
#include <vector>
#include <filesystem>

#define MAX_ENTRIES_PER_FILE 10

class ReportQueue
{
public:
    explicit ReportQueue(const std::string& path); // Constructor with folder path

    void push(const std::string& sentence);
    bool try_pop(std::string& sentence);
    bool empty() const;
    int size() const;
    int size_in_bytes() const;

private:
    std::string generate_new_filename(); // Generates a new filename for each file
    void load_existing_files();          // Loads existing queue files on startup

    mutable std::mutex mutex;
    std::string filename;
    std::string folder_path; // Store the custom folder path
    std::vector<std::string> file_stack; // Stack to store all filenames
    int entry_count;
    int total_entry_count;
    int total_size_in_bytes;
    int file_index;
};

#endif // REPORTQUEUE_H
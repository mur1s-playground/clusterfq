#pragma once

#include <string>
#include <vector>

using namespace std;

char* util_issue_command(const char* cmd);

void util_file_write_line(const string filepath, const char* line);
void util_file_write_line(const string filepath, const string line);
void util_file_write_lines(const string filepath, const vector<string> lines);
vector<string> util_file_read_lines(const string filepath, bool trim = false);
void util_directory_create(const string path);
bool util_path_exists(const string path);
unsigned int util_path_id_get(const string base_path);

void util_sleep(const unsigned int milliseconds);
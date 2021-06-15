#pragma once

#include <string>
#include <vector>
#include <time.h>

using namespace std;

char* util_issue_command(const char* cmd);

void util_file_write_line(const string filepath, const char* line);
void util_file_write_line(const string filepath, const string line);
void util_file_write_lines(const string filepath, const vector<string> lines);
vector<string> util_file_read_lines(const string filepath, bool trim = false);
void util_file_read_binary(string filename, unsigned char* bin, size_t* out_length);
void util_file_write_binary(string filename, unsigned char* bin, size_t length);
void util_directory_create(const string path);
void util_file_delete(const string path);
bool util_path_exists(const string path);
unsigned int util_path_id_get(const string base_path);
string util_file_get_ext(const string path);
string util_file_get_name(const string path);
vector<string> util_file_get_all_names(const string path, time_t timerange_start = 0, time_t timerange_end = 0);

vector<string>	util_split(const std::string& str, const std::string& separator);
void util_sleep(const unsigned int milliseconds);
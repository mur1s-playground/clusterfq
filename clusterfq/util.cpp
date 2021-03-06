#include "util.h"

#include <fstream>
#include <sys/stat.h>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#else
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <dirent.h>
#endif

char* util_issue_command(const char* cmd) {
	char* result;
	char buffer[256];
	int i;
	stringstream ss;
#ifdef _WIN32
	FILE* pipe = _popen(cmd, "r");
	if (pipe != nullptr) {
		while (fgets(buffer, sizeof buffer, pipe) != NULL) {
			ss << buffer;
		}
		_pclose(pipe);
	}
#else
	FILE* pipe;
	pipe = (FILE*)popen(cmd, "r");
	if (!pipe) return NULL;
	while (!feof(pipe)) {
		if (fgets(buffer, 256, pipe) != NULL) {
			for (i = 0; i < 256; i++) {
				if (buffer[i] == '\0') break;
				ss << buffer[i];
			}
		}
	}
	pclose(pipe);
#endif
	result = (char*)malloc(ss.str().length() + 1);
	memcpy(result, ss.str().data(), ss.str().length());
	result[ss.str().length()] = '\0';
	return result;
}

string& util_ltrim(string& str, const string& chars) {
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

string& util_rtrim(string& str, const string& chars) {
	str.erase(str.find_last_not_of(chars) + 1);
	return str;
}

string& util_trim(string& str, const string& chars) {
	return util_ltrim(util_rtrim(str, chars), chars);
}

void util_file_rename(const string from, const string to) {
	rename(from.c_str(), to.c_str());
}

void util_file_write_line(const string filepath, const char* line) {
	string l(line);
	util_file_write_line(filepath, l);
}

void util_file_write_line(const string filepath, const string line) {
	vector<string> v = vector<string>();
	v.push_back(line);
	util_file_write_lines(filepath, v);
}

void util_file_write_lines(const string filepath, const vector<string> lines) {
	ofstream file(filepath);
	if (file.is_open()) {
		for (int l = 0; l < lines.size(); l++) {
			file << lines[l] << std::endl;
		}
		file.close();
	}
}

vector<string> util_file_read_lines(const string filepath, bool trim) {
	vector<string> result;

	ifstream file(filepath);
	string filecontent;
	string chars(" \n\r\t");
	if (file.is_open()) {
		while (getline(file, filecontent)) {
			if (filecontent.size() > 0) {
				string name = filecontent;
				if (trim) {
					name = util_trim(name, chars);
				}
				result.push_back(name);
			}
		}
	}
	return result;
}

void util_file_read_binary(string filename, unsigned char* bin, size_t* out_length) {
#ifdef _WIN32
	HANDLE file_handle = CreateFileA(filename.c_str(),
		FILE_GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	(*out_length) = 0;

	if (file_handle != INVALID_HANDLE_VALUE) {
		char buffer[1024];
		memset(buffer, 0, 1024);

		DWORD dwBytesRead;

		unsigned char* bin_tmp = bin;

		while (ReadFile(file_handle, buffer, 1024, &dwBytesRead, NULL)) {
			if (dwBytesRead != 0) {
				memcpy(bin_tmp, buffer, dwBytesRead);
				bin_tmp += dwBytesRead;
				(*out_length) += dwBytesRead;
			}
			else {
				break;
			}
		}
	}

	CloseHandle(file_handle);
#else
	char buffer[1024];
	memset(buffer, 0, 1024);

	unsigned int bytes_read = 0;

	ifstream filehandle(filename.c_str(), ios::out | ios::binary);
	while (filehandle.read(buffer, 1024)) {
		memcpy(&bin[bytes_read], buffer, 1024);
		bytes_read += 1024;
	}
	unsigned int overlap = filehandle.gcount();
	memcpy(&bin[bytes_read], buffer, overlap);
	bytes_read += overlap;
	*out_length = bytes_read;
	filehandle.close();
#endif
}

void util_file_write_binary(string filename, unsigned char* bin, size_t length) {
#ifdef _WIN32
	HANDLE file_handle = CreateFileA(filename.c_str(),
		FILE_GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	char buffer[1024];
	memset(buffer, 0, 1024);

	DWORD dwBytesWritten;

	int ct = 0;
	while (ct < length) {
		int bytes_to_write = 1024;
		if (length - ct < 1024) {
			bytes_to_write = length - ct;
		}
		memcpy(buffer, &bin[ct], bytes_to_write);
		int part_write = 0;
		while (part_write < bytes_to_write) {
			WriteFile(file_handle, &buffer[part_write], bytes_to_write - part_write, &dwBytesWritten, NULL);
			part_write += dwBytesWritten;
			ct += dwBytesWritten;
		}
	}
	CloseHandle(file_handle);
#else
	ofstream filehandle(filename.c_str(), ios::out | ios::binary);
	filehandle.write((char*)bin, length);
	filehandle.flush();
	filehandle.close();
#endif
}

void util_directory_delete(const string path) {
#ifdef _WIN32
	RemoveDirectoryA(path.c_str());
#else
	rmdir(path.c_str());
#endif
}

void util_directory_create(const string path) {
#ifdef _WIN32
	_mkdir(path.c_str());
#else
	printf("making dir\n");
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

void util_file_delete(const string path) {
	remove(path.c_str());
}

bool util_path_exists(const string path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

unsigned int util_path_id_get(const string base_path) {
	unsigned int f_id = 0;
	while (true) {
		stringstream id_path;
		id_path << base_path << f_id << "/";

		if (!util_path_exists(id_path.str())) {
			break;
		}
		f_id++;
	}
	return f_id;
}

string util_file_get_ext(const string path) {
	vector<string> splt = util_split(path, ".");
	if (splt.size() > 1) {
		stringstream ext;
		ext << "." << splt[splt.size() - 1];
		string res = ext.str();
		string chars = "/\\\n\r\t ";
		res = util_trim(res, chars);
		return res;
	}
	return "";
}

string util_file_get_name(const string path) {
	vector<string> splt = util_split(path, "/");
	if (splt.size() <= 1) {
		splt = util_split(path, "\\");
	}
	stringstream name;
	name << splt[splt.size() - 1];
	string res = name.str();
	string chars = "/\\\n\r\t ";
	res = util_trim(res, chars);
	return res;
}

void util_file_get_all_names_inner(vector<string>* filenames, string fname, time_t timerange_start, time_t timerange_end) {
	bool take_file = true;

	vector<string> splt = util_split(fname, ".");
	time_t time_ = stol(splt[0]);
	if (timerange_start > 0 && time_ < timerange_start) {
		take_file = false;
	}
	if (timerange_end > 0 && time_ > timerange_end) {
		take_file = false;
	}
	if (take_file) {
		filenames->push_back(fname);
	}
}

vector<string> util_file_get_all_names(const string path, time_t timerange_start, time_t timerange_end, bool timerange) {
	vector<string> filenames = vector<string>();
#ifdef _WIN32
	WIN32_FIND_DATAA data;

	stringstream suf;
	suf << path << "\\*";

	HANDLE h = FindFirstFileA(suf.str().c_str(), &data);
	do {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

		} else {
			string fname(data.cFileName);
			if (timerange) {
				util_file_get_all_names_inner(&filenames, fname, timerange_start, timerange_end);
			} else {
				filenames.push_back(fname);
			}
		}
	} while (FindNextFileA(h, &data));
	FindClose(h);
#else
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(path.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strstr(ent->d_name, "..") != ent->d_name && strstr(ent->d_name, ".") != ent->d_name) {
				string fname(ent->d_name);
				if (timerange) {
					util_file_get_all_names_inner(&filenames, fname, timerange_start, timerange_end);
				} else {
					filenames.push_back(fname);
				}

			}
		}
		closedir(dir);
	}
#endif
	return filenames;
}

vector<string>	util_split(const std::string& str, const std::string& separator) {
	vector<string> result;
	int start = 0;
	int end = str.find_first_of(separator, start);
	while (end != std::string::npos) {
		result.push_back(str.substr(start, end - start));
		start = end + 1;
		end = str.find_first_of(separator, start);
	}
	result.push_back(str.substr(start));
	return result;
}

void util_sleep(const unsigned int milliseconds) {
#ifdef _WIN32
	Sleep(milliseconds);
#else
	usleep(milliseconds * 1000);
#endif
}

void util_ipv6_address_to_normalform(char* pbar) {
	char part_2[46];
	//memset(&part_2, 'x', 46);
	part_2[45] = '\0';

	int part_2_pos = 44;

	char* end = strchr(pbar, '\0');
	end--;
	int segment_ct = 0;
	int segs = 0;

	bool dbl = false;
	while (end >= pbar) {
		if (*end == ':') {
			if (*(end - 1) == ':') {
				dbl = true;
			}
			else if (segment_ct > 0) {
				for (int s = segment_ct; s < 4; s++) {
					part_2[part_2_pos] = '0';
					part_2_pos--;
				}
			}
			part_2[part_2_pos] = *end;
			part_2_pos--;
			segment_ct = 0;
			segs++;
		}
		else {
			part_2[part_2_pos] = *end;
			part_2_pos--;
			segment_ct = (segment_ct + 1) % 4;
		}
		end--;
		if (dbl) break;
	}
	//printf("part_2: %s\n", part_2);

	part_2_pos++;
	if (dbl) {
		char part_1[46];
		//memset(&part_1, 'x', 46);
		int part_1_pos = 44;
		part_1[45] = '\0';
		while (end >= pbar) {
			if (*end == ':') {
				if (segment_ct > 0) {
					for (int s = segment_ct; s < 4; s++) {
						part_1[part_1_pos] = '0';
						part_1_pos--;
					}
				}
				part_1[part_1_pos] = *end;
				part_1_pos--;
				segment_ct = 0;
				segs++;
			}
			else {
				part_1[part_1_pos] = *end;
				part_1_pos--;
				segment_ct = (segment_ct + 1) % 4;
			}
			end--;
		}
		//printf("part_1: %s\n", part_1);
		part_1_pos++;

		int total_pos = 0;
		memcpy(pbar, &part_1[part_1_pos], 45 - part_1_pos);
		total_pos = 45 - part_1_pos;
		for (int s = segs; s < 8; s++) {
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			if (s < 7) {
				pbar[total_pos] = ':';
				total_pos++;
			}
		}
		memcpy(&pbar[total_pos], &part_2[part_2_pos], 45 - part_2_pos);
		total_pos += (45 - part_2_pos);
		pbar[total_pos] = '\0';
	}
	else {
		memcpy(pbar, &part_2[part_2_pos], 45 - part_2_pos);
		pbar[45 - part_2_pos] = '\0';
	}
}
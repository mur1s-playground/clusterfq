#include "util.h"

#include <fstream>
#include <sys/stat.h>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

char* util_issue_command(const char* cmd) {
	char* result;
	char buffer[256];
	int i;
#ifdef _WIN32
	result = NULL;
#else
	FILE* pipe;
	result = (char*)malloc(sizeof(char) * 256);
	pipe = (FILE*)popen(cmd, "r");
	if (!pipe) return NULL;
	while (!feof(pipe)) {
		if (fgets(buffer, 256, pipe) != NULL) {
			for (i = 0; i < 256; i++) {
				result[i] = buffer[i];
				if (buffer[i] == '\0') break;
			}
		}
	}
	pclose(pipe);
#endif
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

void util_directory_create(const string path) {
#ifdef _WIN32
	_mkdir(path.c_str());
#else
	printf("making dir\n");
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
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

void util_sleep(const unsigned int milliseconds) {
#ifdef _WIN32
	Sleep(milliseconds);
#else
	usleep(milliseconds * 1000);
#endif
}
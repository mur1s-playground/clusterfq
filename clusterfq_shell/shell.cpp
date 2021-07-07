#include "shell.h"

#include <fstream>
#include <iostream>
#include <sstream>

std::vector<std::vector<std::string>> session_macros;
std::vector<std::vector<std::string>>	shell_cmd_queue;

struct mutex	shell_cmd_lock;

int parseMacro(std::string input);
void explodeMacro(std::string& command);
void split(const std::string& str, std::vector<std::string>& v);

void shell_loop(void* param) {
	mutex_init(&shell_cmd_lock);
	int daemon = 1;
	std::string input;
	while (daemon == 1) {
		std::cout << "clusterfq> ";
		std::getline(std::cin, input);
		daemon = parseMacro(input);
	}
}

int parseCommand(std::string& command) {
	int daemon = 1;
	std::vector<std::string> args;
	split(command, args);
	if (args.size() > 0) {
		if (args[0] == "exit" || args[0] == "quit") {
			daemon = 0;
		}
		else {
			mutex_wait_for(&shell_cmd_lock);
			shell_cmd_queue.push_back(args);
			mutex_release(&shell_cmd_lock);
		}
	}
	return daemon;
}

int parseMacro(std::string input) {
	int daemon = 1;
	std::vector<std::string> splt;
	split(input, splt);
	if (splt.size() > 0) {
		if (splt[0] == "load") {
			if (splt.size() < 2) {
				std::cout << "usage: load filename\n";
			}
			else {
				std::ifstream file(splt[1]);
				std::string str;
				while (std::getline(file, str)) {
					parseMacro(str);
				}
			}
		}
		else if (splt[0] == "define") {
			bool found = false;
			int i;
			for (i = 0; i < session_macros.size(); i++) {
				if (session_macros[i][0] == splt[1]) {
					break;
				}
			}
			std::cout << "defined new command " << splt[1] << " as ";
			if (i < session_macros.size()) {
				session_macros[i].erase(session_macros[i].begin() + 1);
			}
			else {
				std::vector<std::string> new_macro;
				new_macro.push_back(splt[1]);
				session_macros.push_back(new_macro);
			}
			session_macros[i].push_back(input.substr(splt[0].size() + 1 + splt[1].size() + 1));
			for (i = 2; i < splt.size(); i++) {
				std::cout << splt[i] << " ";
			}
			std::cout << "\n";
		}
		else {
			std::string command = input;
			std::string old_cmd;
			do {
				old_cmd = command;
				explodeMacro(command);
				std::cout << "exploded " << old_cmd << " to " << command << "\n";
			} while (old_cmd != command);
			std::istringstream ss(command);
			while (std::getline(ss, command, ';') && daemon == 1) {
				daemon = parseCommand(command);
			}
		}
	}
	return daemon;
}

void explodeMacro(std::string& command) {
	std::istringstream ss(command);
	std::string sub_cmd;
	std::stringstream res;
	while (std::getline(ss, sub_cmd, ';')) {
		res << ";";
		bool found = false;
		for (int i = 0; i < session_macros.size(); i++) {
			if (session_macros[i][0] == sub_cmd) {
				res << session_macros[i][1];
				found = true;
				break;
			}
		}
		if (!found) {
			res << sub_cmd;
		}
	}
	command = res.str().substr(1);
}

void split(const std::string& str, std::vector<std::string>& v) {
	std::stringstream ss(str);
	ss >> std::noskipws;
	std::string field;
	char ws_delim;
	while (1) {
		if (ss >> field)
			v.push_back(field);
		else if (ss.eof())
			break;
		else
			v.push_back(std::string());
		ss.clear();
		ss >> ws_delim;
	}
}
#pragma once

#include <vector>
#include <string>
#include <map>

using namespace std;

struct paramset {
	int									param_count;
	vector<pair<const char*, string>>	params;
};

extern map<int, struct paramset>		paramsets;


void paramset__static_init();

int paramset__create();

bool paramset__param_add(int id, const char* param, int value);
bool paramset__param_add(int id, const char* param, long long value);
bool paramset__param_add(int id, const char* param, string value);
void paramset__remove(int id);

struct paramset* paramset__get(int id);
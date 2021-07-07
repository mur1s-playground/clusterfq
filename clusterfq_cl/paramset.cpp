#include "paramset.h"

#include "../clusterfq/mutex.h"

#include <sstream>

map<int, struct paramset>		paramsets;
struct mutex					paramsets_lock;

void paramset__static_init() {
	mutex_init(&paramsets_lock);
}

int paramset__create() {
	mutex_wait_for(&paramsets_lock);
	int f_id = -1;
	for (int i = 0; i < 20; i++) {
		if (paramsets.find(i) == paramsets.end()) {
			struct paramset ps;
			ps.param_count = 0;
			ps.params = vector<pair<const char*, string>>();
			paramsets.insert(pair<int, struct paramset>(i, ps));
			f_id = i;
		}
	}
	mutex_release(&paramsets_lock);
	return f_id;
}

bool paramset__param_add(int id, const char* param, int value) {
	stringstream ss;
	ss << value;
	return paramset__param_add(id, param, ss.str());
}

bool paramset__param_add(int id, const char* param, long long value) {
	stringstream ss;
	ss << value;
	return paramset__param_add(id, param, ss.str());
}

bool paramset__param_add(int id, const char* param, string value) {
	mutex_wait_for(&paramsets_lock);
	map<int, struct paramset>::iterator psi = paramsets.find(id);
	struct paramset* ps = nullptr;
	if (psi != paramsets.end()) {
		ps = &psi->second;
	}
	mutex_release(&paramsets_lock);
	if (ps == nullptr) {
		return false;
	} else {
		ps->params.push_back(pair<const char*, string>(param, value));
		ps->param_count++;
	}
	return true;
}

void paramset__remove(int id) {
	mutex_wait_for(&paramsets_lock);
	map<int, struct paramset>::iterator psi = paramsets.find(id);
	if (psi != paramsets.end()) {
		paramsets.erase(psi);
	}
	mutex_release(&paramsets_lock);
}

struct paramset* paramset__get(int id) {
	mutex_wait_for(&paramsets_lock);
	map<int, struct paramset>::iterator psi = paramsets.find(id);
	struct paramset* ps = nullptr;
	if (psi != paramsets.end()) {
		ps = &psi->second;
	}
	mutex_release(&paramsets_lock);
	return ps;
}
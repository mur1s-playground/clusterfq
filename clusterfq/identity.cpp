#include "identity.h"

#include "address_factory.h"
#include "contact.h"
#include "util.h"

#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#endif

vector<struct identity>		identities = vector<struct identity>();

void identity_create(struct identity* i, string name) {
	i->name = name;

	struct Key k;
	crypto_key_init(&k);
	crypto_key_private_generate(&k, 2048);
	crypto_key_public_extract(&k);
	crypto_key_name_set(&k, name.c_str(), name.length());
	i->keys.push_back(k);
	i->key_latest_creation = time(nullptr);

	identity_create_fs(i);
}

void identity_create_fs(struct identity* i) {
	stringstream base_dir;
	base_dir << "./identities/";

	util_directory_create(base_dir.str());

	unsigned int f_id = util_path_id_get(base_dir.str());

	i->id = f_id;

	base_dir << f_id << "/";
	util_directory_create(base_dir.str());

	stringstream name_path;
	name_path << base_dir.str() << "name";
	util_file_write_line(name_path.str(), i->name);

	identity_save_keys(i, base_dir.str());

	stringstream contacts_dir;
	contacts_dir << base_dir.str() << "contacts/";
	util_directory_create(contacts_dir.str());

	stringstream groups_dir;
	groups_dir << base_dir.str() << "groups/";
	util_directory_create(groups_dir.str());
}

void identity_load(unsigned int id) {
	struct identity i;
	identity_load(&i, id);
	identities.push_back(i);

	for (int c = 0; c < identities[identities.size() - 1].contacts.size(); c++) {
		struct contact* cc = &identities[identities.size() - 1].contacts[c];

		stringstream base_path;
		base_path << "./identities/" << i.id << "/contacts/" << cc->id << "/out/";

		vector<string> filenames = util_file_get_all_names(base_path.str(), 0, 0, false);
		for (int f = 0; f < filenames.size(); f++) {
			vector<string> splt = util_split(filenames[f], ".");

			const char* fn = filenames[f].c_str();
			if (strstr(fn, ".pending") == fn + filenames[f].length() - strlen(".pending")) {
				enum message_type mt = MT_MESSAGE;				

				string name = "";
				unsigned char* d;
				size_t d_len = 0;
				time_t time_pending = stoi(splt[0]);

				size_t b64_out = 0;
				stringstream b64_hash_id;
				b64_hash_id << splt[1] << "\n";

				unsigned char* hash_id = crypto_base64_decode(b64_hash_id.str().c_str(), &b64_out, true);

				stringstream fullpath;
				fullpath << base_path.str() << filenames[f];

				if (strstr(fn, ".file.") != nullptr) {
					stringstream f_ss;
					for (int fna = 3; fna < splt.size() - 1; fna++) {
						f_ss << splt[fna];
						if (fna + 1 < splt.size() - 1) f_ss << ".";
					}
					name = f_ss.str();

					mt = MT_FILE;
					d = (unsigned char*)malloc(10 * 1024 * 1024);
					util_file_read_binary(fullpath.str(), d, &d_len);
				} else {
					stringstream data;
					vector<string> lines = util_file_read_lines(fullpath.str());

					for (int l = 0; l < lines.size(); l++) {
						if (l > 0) data << "\\n";
						data << lines[l];
					}
					d = (unsigned char*)malloc(data.str().length() + 1);
					memcpy(d, data.str().data(), data.str().length());
					d[data.str().length()] = '\0';
					d_len = data.str().length();
				}
				message_resend_pending(&identities[identities.size() - 1], cc, mt, time_pending, name, d, d_len, hash_id);
			}
		}
	}
}

void identity_load(struct identity *i, unsigned int id) {
	stringstream base_dir;
	base_dir << "./identities/" << id << "/";

	i->id = id;

	stringstream name_path;
	name_path << base_dir.str() << "name";
	vector<string> name = util_file_read_lines(name_path.str(), true);
	i->name = name[0];

	identity_load_keys(i, base_dir.str());

	unsigned int c_id = 0;
	stringstream contacts_path;
	contacts_path << base_dir.str() << "contacts/";

	while (true) {
		stringstream contact_path;
		contact_path << contacts_path.str() << c_id << "/";

		if (util_path_exists(contact_path.str())) {
			struct contact c;
			contact_load(&c, id, c_id, contact_path.str());
			i->contacts.push_back(c);
		}
		
		c_id++;
		//TODO: improve
		if (c_id == 100) break;
	}
}

void identity_migrate_key(struct identity* i, unsigned int bits) {
	struct Key k;
	crypto_key_init(&k);
	crypto_key_name_set(&k, i->name.c_str(), i->name.length());;
	crypto_key_private_generate(&k, (int)bits);
	crypto_key_public_extract(&k);
	i->keys.push_back(k);
	i->key_latest_creation = time(nullptr);
	identity_save_keys(i);
	for (int c = 0; c < i->contacts.size(); c++) {
		message_send_migrate_key(i, &i->contacts[c]);
	}
}

void identity_remove_obsolete_keys(struct identity* i) {
	bool* used_keys = (bool*)malloc(i->keys.size() * sizeof(bool));
	memset(used_keys, 0, i->keys.size() * sizeof(bool));
	for (int c = 0; c < i->contacts.size(); c++) {
		struct contact* con = &i->contacts[c];
		used_keys[con->identity_key_id] = true;
		mutex_wait_for(&con->outgoing_messages_lock);
	}
	int removed = 0;
	for (int k = 0; k < i->keys.size(); k++) {
		if (used_keys[k] == false) {
			for (int c = 0; c < i->contacts.size(); c++) {
				struct contact* con = &i->contacts[c];
				if (con->identity_key_id > k - removed) {
					con->identity_key_id--;
				}
			}
			removed++;
		}
	}
	for (int k = i->keys.size() - 1; k >= 0; k--) {
		if (used_keys[k] == false) {
			i->keys.erase(i->keys.begin() + k);
		}
	}
	free(used_keys);
	for (int c = 0; c < i->contacts.size(); c++) {
		struct contact* con = &i->contacts[c];
		contact_identity_key_id_save(con, i->id);
		mutex_release(&con->outgoing_messages_lock);
	}
	identity_save_keys(i);
	for (int k = i->keys.size(); k < i->keys.size() + removed; k++) {
		stringstream key_path;
		key_path << "./identities/" << i->id << "/key_" << k;
		util_file_delete(key_path.str());
	}
}

void identity_save_keys(struct identity* i) {
	stringstream base_dir;
	base_dir << "./identities/" << i->id << "/";
	identity_save_keys(i, base_dir.str());
}

void identity_save_keys(struct identity* i, string base_dir) {
	for (int k = 0; k < i->keys.size(); k++) {
		stringstream key_path;
		key_path << base_dir << "key_" << k;
		util_file_write_line(key_path.str(), i->keys[k].private_key);
	}
	stringstream latest_path;
	latest_path << base_dir << "key_latest_time";

	stringstream time_latest;
	time_latest << i->key_latest_creation;

	util_file_write_line(latest_path.str(), time_latest.str());
}

void identity_load_keys(struct identity* i, string base_dir) {
	unsigned int k_id = 0;
	while (true) {
		stringstream key_path;
		key_path << base_dir << "key_" << k_id;
		vector<string> priv_key = util_file_read_lines(key_path.str(), false);

		if (priv_key.size() > 0) {
			stringstream prk;
			for (int pk = 0; pk < priv_key.size(); pk++) {
				prk << priv_key[pk] << "\n";
			}
			string prk_s = prk.str();
			const char* prk_c = prk_s.c_str();

			struct Key k;
			crypto_key_init(&k);
			crypto_key_name_set(&k, i->name.c_str(), i->name.length());
			k.private_key = (char*)malloc(prk.str().length() + 1);
			memcpy(k.private_key, prk_c, prk.str().length());
			k.private_key[prk.str().length()] = '\0';
			k.private_key_len = prk.str().length() + 1;

			crypto_key_public_extract(&k);

			//crypto_key_dump(&k);

			i->keys.push_back(k);
		}
		k_id++;
		//TODO: improve
		if (k_id == 100) break;
	}

	stringstream latest_path;
	latest_path << base_dir << "key_latest_time";

	vector<string> latest_time = util_file_read_lines(latest_path.str(), true);
	if (latest_time.size() > 0) {
		i->key_latest_creation = stoi(latest_time[0]);
	} else {
		i->key_latest_creation = 0;
	}
}

void identity_delete(unsigned int identity_id) {
	struct identity* i = identity_get(identity_id);
	
	if (i != nullptr) {
		stringstream base_dir;
		base_dir << "./identities/" << identity_id << "/";

		for (int c = i->contacts.size() - 1; c >= 0; c--) {
			contact_delete(&i->contacts[c], identity_id);
			i->contacts.erase(i->contacts.begin() + c);
		}
		stringstream contacts_dir;
		contacts_dir << base_dir.str() << "contacts/";
		util_directory_delete(contacts_dir.str());

		stringstream groups_dir;
		groups_dir << base_dir.str() << "groups/";
		util_directory_delete(groups_dir.str());

		vector<string> base_files = util_file_get_all_names(base_dir.str(), 0, 0, false);
		for (int i = 0; i < base_files.size(); i++) {
			stringstream full_path;
			full_path << base_dir.str() << base_files[i];

			util_file_delete(full_path.str());
		}
		util_directory_delete(base_dir.str());

		//TODO: memory cleanup

		for (int i = 0; i < identities.size(); i++) {
			if (identities[i].id == identity_id) {
				identities.erase(identities.begin() + i);
				break;
			}
		}
	}
}

void identities_load() {
	stringstream base_dir;
	base_dir << "./identities/";

	unsigned int f_id = 0;
	while (true) {
		stringstream id_path;
		id_path << base_dir.str() << f_id << "/";

		if (util_path_exists(id_path.str())) {
			if (identity_get(f_id) == nullptr) {
				identity_load(f_id);
			}
		}
		f_id++;
		//TODO: improve
		if (f_id == 100) break;
	}
}

string identities_list() {
	stringstream result;
	result << "{\n";
	result << "\t\"identities\": [\n";
	for (int i = 0; i < identities.size(); i++) {
		result << "\t\t{\n";
		result << "\t\t\t\"id\": "		<< identities[i].id		<< ",\n";
		result << "\t\t\t\"name\": \""	<< identities[i].name	<< "\",\n";
		result << "\t\t\t\"keys\": [\n";
		for (int k = 0; k < identities[i].keys.size(); k++) {
			result << "\t\t\t\t{\n";
			result << "\t\t\t\t\t\"pubkey\": \""; 
			for (int l = 0; l < identities[i].keys[k].public_key_len-1; l++) {
				if (identities[i].keys[k].public_key[l] == '\n') {
					result << "\\n";
				} else {
					result << identities[i].keys[k].public_key[l];
				}
				
			}
			result << "\",\n";
			result << "\t\t\t\t\t\"fingerprint\": \"";
			if (identities[i].keys[k].public_key_len > 0) {
				char* fingerprint = crypto_key_fingerprint(&identities[i].keys[k]);
				string fng(fingerprint);
				result << fng;
				free(fingerprint);
			}
			result << "\"\n";
			result << "\t\t\t\t}";
			if (k + 1 < identities[i].keys.size()) {
				result << ",";
			}
			result << "\n";
		}
		result << "\t\t\t]\n";
		result << "\t\t}";
		if (i + 1 < identities.size()) {
			result << ",";
		}
		result << "\n";
	}
	result << "\t]\n";
	result << "}\n";
	string res = result.str();
	return res;
}

struct identity *identity_get(unsigned int id) {
	for (int i = 0; i < identities.size(); i++) {
		if (identities[i].id == id) {
			return &identities[i];
		}
	}
	return nullptr;
}

void identity_contact_delete(unsigned int id, unsigned int c_id) {
	struct identity* i = identity_get(id);
	if (i != nullptr) {
		for (int c = 0; c < i->contacts.size(); c++) {
			if (i->contacts[c].id == c_id) {
				contact_delete(&i->contacts[c], i->id);
				i->contacts.erase(i->contacts.begin() + c);
				break;
			}
		}
	}
}

string identity_contact_verify(unsigned int identity_id, unsigned int contact_id) {
	struct identity* i = identity_get(identity_id);
	if (i == nullptr) return "{ }";
	struct contact* c = contact_get(&i->contacts, contact_id);
	if (c == nullptr) return "{ }";
	c->verified = true;

	stringstream base_dir;
	base_dir << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_verified_save(c, base_dir.str());

	stringstream result;
	result << "{\n";
	result << "\t\"identity_id\": " << identity_id << ",\n";
	result << "\t\"contact_id\": " << contact_id << "\n";
	result << "}\n";
	string res = result.str();
	return res;
}

void identity_contact_add(unsigned int id, struct contact* c) {
	struct identity* i = identity_get(id);
	if (i != nullptr) {
		stringstream base_dir;
		base_dir << "./identities/" << id << "/contacts/";

		unsigned int f_id = util_path_id_get(base_dir.str());
		c->id = f_id;
		c->identity_key_id = i->keys.size() - 1;
		
		i->contacts.push_back(*c);

		stringstream contact_path;
		contact_path << base_dir.str() << c->id << "/";
		util_directory_create(contact_path.str());

		stringstream contact_msg_in;
		contact_msg_in << contact_path.str() << "in/";
		util_directory_create(contact_msg_in.str());

		stringstream contact_msg_out;
		contact_msg_out << contact_path.str() << "out/";
		util_directory_create(contact_msg_out.str());

		contact_save(c, contact_path.str());
	}
}

string identity_share(unsigned int id, string name_to) {
	stringstream result;

	result << "{\n";
	result << "\t\"identity_id\": " << id << ",\n";
	result << "\t\"name_to\": \"" << name_to << "\",\n";
	result << "\t\"identity_share\": {\n";

	struct identity* i = identity_get(id);
	if (i != nullptr) {
		string address_rev = address_factory_get_unique();

		result << "\t\t\"name\": \"" << i->name << "\",\n";
		result << "\t\t\"pubkey\": \"";
		for (int l = 0; l < i->keys[i->keys.size() - 1].public_key_len - 1; l++) {
			if (i->keys[i->keys.size() - 1].public_key[l] == '\n') {
				result << "\\n";
			} else {
				result << i->keys[i->keys.size() - 1].public_key[l];
			}
		}
		result << "\",\n";
		result << "\t\t\"address\": \"" << address_rev << "\"\n";
		
		struct contact c;

		stringstream base_dir;
		base_dir << "./identities/" << id << "/contacts/";
		unsigned int f_id = util_path_id_get(base_dir.str());

		c.id = f_id;
		contact_create_open(&c, name_to, address_rev);
		c.identity_key_id = i->keys.size() - 1;

		stringstream contact_path;
		contact_path << base_dir.str() << c.id << "/";
		util_directory_create(contact_path.str());

		stringstream contact_msg_in;
		contact_msg_in << contact_path.str() << "in/";
		util_directory_create(contact_msg_in.str());

		stringstream contact_msg_out;
		contact_msg_out << contact_path.str() << "out/";
		util_directory_create(contact_msg_out.str());

		contact_save(&c, contact_path.str());

		address_factory_add_address(address_rev, AFST_CONTACT, id, c.id);
		i->contacts.push_back(c);
	}
	result << "\t}\n";
	result << "}\n";
	string res = result.str();
	return res;
}

string identity_contact_list(unsigned int id) {
	stringstream result;
	struct identity* i = identity_get(id);
	result << "{\n";
	result << "\t\"identity_id\": " << id << ",\n";
	result << "\t\"contacts\": [\n";

	if (i != nullptr) {
		for (int c = 0; c < i->contacts.size(); c++) {
			result << "\t\t{\n";
			result << "\t\t\t\"id\": " << i->contacts[c].id << ",\n";
			result << "\t\t\t\"name\": \"" << i->contacts[c].name << "\",\n";

			tm* gmtm = gmtime(&i->contacts[c].cs->last_seen);

			string gmt(asctime(gmtm));
			gmt = util_trim(gmt, "\r\n\t ");

			result << "\t\t\t\"last_seen\": \"" << gmt << " UTC" << "\",\n";
			result << "\t\t\t\"address_available\": " << (i->contacts[c].address.length() > 0) << ",\n";
			result << "\t\t\t\"fingerprint\": \"";
			if (i->contacts[c].pub_key.public_key_len > 0) {
				char* fingerprint = crypto_key_fingerprint(&i->contacts[c].pub_key);
				string fng(fingerprint);
				result << fng;
				free(fingerprint);
			}
			result << "\",\n";
			result << "\t\t\t\"verified\": " << i->contacts[c].verified << "\n";
			result << "\t\t}";
			if (c + 1 < i->contacts.size()) {
				result << ",";
			}
			result << "\n";
			//contact_dump(&i->contacts[c]);
		}
	}

	result << "\t]";
	result << "}\n";
	string res = result.str();
	return res;
}

string identity_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code) {
	string content = "{ }";
	const char* request_action = nullptr;
	if (request_path->size() > 1) {
		request_action = (*request_path)[1].c_str();

		if (sirt == SIRT_GET) {
			if (strstr(request_action, "list") == request_action) {
				content = identities_list();
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "contact_list") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				content = identity_contact_list(identity_id);
				*status_code = (char*)HTTP_RESPONSE_200;
			} else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		} else if (sirt == SIRT_POST) {
			if (strstr(request_action, "load_all") == request_action) {
				identities_load();
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "create") == request_action) {
				string name = http_request_get_param(request_params, "name");
				if (name.length() > 0) {
					struct identity i;
					identity_create(&i, name);
					identities.push_back(i);
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "contact_add") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				string name = http_request_get_param(request_params, "name");
				string address = http_request_get_param(request_params, "address");
				string pubkey(post_content);
				if (name.length() > 0 && address.length() > 0 && pubkey.length() > 0) {
					struct contact c;
					contact_create(&c, name, pubkey, address);
					identity_contact_add(identity_id, &c);
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "contact_delete") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				identity_contact_delete(identity_id, contact_id);
				stringstream content_ss;
				content_ss << "{\n\t\"identity_id\": " << identity_id << ",\n\t\"contact_id\": " << contact_id << "}\n";
				content = content_ss.str();
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "contact_verify") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				content = identity_contact_verify(identity_id, contact_id);
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "share") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				string name = http_request_get_param(request_params, "name");
				if (name.length() > 0) {
					content = identity_share(identity_id, name);
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "migrate_key") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				struct identity* i = identity_get(identity_id);
				if (i != nullptr) {
					identity_migrate_key(i, 2048);
					stringstream content_ss;
					content_ss << "{\n\t\"identity_id\": " << identity_id << "\n}\n";
					content = content_ss.str();
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "remove_obsolete_keys") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				struct identity* i = identity_get(identity_id);
				if (i != nullptr) {
					identity_remove_obsolete_keys(i);
					stringstream content_ss;
					content_ss << "{\n\t\"identity_id\": " << identity_id << "\n}\n";
					content = content_ss.str();
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "delete") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				identity_delete(identity_id);
				stringstream content_ss;
				content_ss << "{\n\t\"identity_id\": " << identity_id << "\n}\n";
				content = content_ss.str();
				*status_code = (char*)HTTP_RESPONSE_200;
			} else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		} else if (sirt == SIRT_OPTIONS) {
			content = "Access-Control-Allow-Headers: Content-Type\n";
			*status_code = (char*)HTTP_RESPONSE_200;
		}
	}
	return content;
}
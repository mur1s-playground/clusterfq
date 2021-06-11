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
	crypto_key_private_generate(&k, 2048);
	crypto_key_public_extract(&k);
	crypto_key_name_set(&k, name.c_str(), name.length());
	i->keys.push_back(k);

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

	for (int k = 0; k < i->keys.size(); k++) {
		stringstream key_path;
		key_path << base_dir.str() << "key_" << k;
		util_file_write_line(key_path.str(), i->keys[k].private_key);
	}

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
}

void identity_load(struct identity *i, unsigned int id) {
	stringstream base_dir;
	base_dir << "./identities/" << id << "/";

	i->id = id;

	stringstream name_path;
	name_path << base_dir.str() << "name";
	vector<string> name = util_file_read_lines(name_path.str(), true);
	i->name = name[0];

	unsigned int k_id = 0;
	while (true) {
		stringstream key_path;
		key_path << base_dir.str() << "key_" << k_id;
		vector<string> priv_key = util_file_read_lines(key_path.str(), false);

		if (priv_key.size() > 0) {
			stringstream prk;
			for (int pk = 0; pk < priv_key.size(); pk++) {
				prk << priv_key[pk] << "\n";
			}
			string prk_s = prk.str();
			const char* prk_c = prk_s.c_str();

			struct Key k;
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

void identities_load() {
	stringstream base_dir;
	base_dir << "./identities/";

	unsigned int f_id = 0;
	while (true) {
		stringstream id_path;
		id_path << base_dir.str() << f_id << "/";

		if (util_path_exists(id_path.str())) {
			identity_load(f_id);
		}
		f_id++;
		//TODO: improve
		if (f_id == 100) break;
	}
}

void identities_list() {
	std::cout << std::endl;
	for (int i = 0; i < identities.size(); i++) {
		std::cout << "/------------------/" << std::endl;
		std::cout << identities[i].id << std::endl;
		std::cout << identities[i].name << std::endl;
		std::cout << "key_count: " << identities[i].keys.size() << std::endl;
	}
	std::cout << "/------------------/" << std::endl;
}

struct identity *identity_get(unsigned int id) {
	for (int i = 0; i < identities.size(); i++) {
		if (identities[i].id == id) {
			return &identities[i];
		}
	}
	return nullptr;
}

void identity_contact_add(unsigned int id, struct contact* c) {
	struct identity* i = identity_get(id);
	if (i != nullptr) {
		stringstream base_dir;
		base_dir << "./identities/" << id << "/contacts/";

		unsigned int f_id = util_path_id_get(base_dir.str());
		c->id = f_id;
		
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

void identity_share(unsigned int id, string name_to) {
	std::cout << std::endl;
	struct identity* i = identity_get(id);
	if (i != nullptr) {
		string address_rev = address_factory_get_unique();
		std::cout << i->name << std::endl;
		std::cout << i->keys[0].public_key << std::endl;
		std::cout << address_rev << std::endl;

		struct contact c;

		stringstream base_dir;
		base_dir << "./identities/" << id << "/contacts/";
		unsigned int f_id = util_path_id_get(base_dir.str());

		c.id = f_id;
		contact_create_open(&c, name_to, address_rev);

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
	std::cout << "/------------------/" << std::endl;
}

void identity_contact_list(unsigned int id) {
	std::cout << std::endl;
	struct identity* i = identity_get(id);
	if (i != nullptr) {
		for (int c = 0; c < i->contacts.size(); c++) {
			contact_dump(&i->contacts[c]);
		}
	}
	std::cout << "/------------------/" << std::endl;
}
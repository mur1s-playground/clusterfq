var Packetset = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.update_interval = 1000;
	this.update_counter = 0;
	
	this.widget = new Widget("Packetset");

	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.changed = true;
	
	this.packetset_info_states = [
		"PS_OUT_CREATED",
		"PS_OUT_PENDING",
		"PS_OUT_COMPLETE",
		"PS_IN_PENDING",
		"PS_IN_COMPLETE"
	];
	
	this.message_types = [
		"MT_ESTABLISH_CONTACT",
		"MT_ESTABLISH_SESSION",
		"MT_MIGRATE_PUBKEY",
		"MT_MIGRATE_ADDRESS",
		"MT_DROP_SESSION",
		"MT_MESSAGE",
		"MT_FILE",
		"MT_RECEIPT",
		"MT_RECEIPT_COMPLETE",
		"MT_UNKNOWN"
	];
	
	this.changed_f = function() {
		this.changed = true;
		if (this.change_dependencies != null) {
			for (var i = 0; i < this.change_dependencies.length; i++) {
				this.change_dependencies[i].changed_f();
			}
		}
	}
	
	this.on_poll_response = function() {
		var poll = JSON.parse(this.responseText);
		
		var state_info_array = poll["packetset_state_infos"];

		var update_contacts_view = false;
		var update_identities_view = false;
		state_info_array.forEach(element => {
			update_identities_view = true;
			
			var p_state = packetset.packetset_info_states[element["packetset_state_info"]];
			var m_type = packetset.message_types[element["message_type"]];
			
			var identity_element_out = document.getElementById(identities.widget.name + "_identity_" + element["identity_id"] + "_sending");
			var identity_element_in = document.getElementById(identities.widget.name + "_identity_" + element["identity_id"] + "_receiving");
			var identity_element_in_done = document.getElementById(identities.widget.name + "_identity_" + element["identity_id"] + "_received");

			if (element["identity_id"] == identities.identity_selected_id) {
				update_contacts_view = true;
			}
			if (p_state == "PS_OUT_PENDING") {
				identities.identities_json["identities"][element["identity_id"]]["PS_OUT_PENDING"] = true;
				contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["PS_IN_PENDING"] = true;
			} else if (p_state == "PS_OUT_COMPLETE") {
				identities.identities_json["identities"][element["identity_id"]]["PS_OUT_PENDING"] = false;
				contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["PS_IN_PENDING"] = false;
			} else if (p_state == "PS_IN_PENDING") {
				identity_element_in.style.backgroundColor = "#00ff00";
				identities.identities_json["identities"][element["identity_id"]]["PS_IN_PENDING"] = true;
				contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["PS_OUT_PENDING"] = true;
			} else if (p_state == "PS_IN_COMPLETE") {
				identities.identities_json["identities"][element["identity_id"]]["PS_IN_PENDING"] = false;
				contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["PS_OUT_PENDING"] = false;
				if (m_type == "MT_MESSAGE" || m_type == "MT_FILE") {
					contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["PS_IN_COMPLETE"] = true;
					if (identities.identity_selected_id != element["identity_id"]) {
						identities.identities_json["identities"][element["identity_id"]]["PS_IN_COMPLETE"] = true;
					}
				}
			}
		});
		
		//TODO:
		// - make update_state_view_functions
		// - maybe call chat update based on polling
		if (update_identities_view) {
			identities.update_identities_view();
		}
		if (update_contacts_view) {
			contacts.update_contacts_view();
		}
		
		packetset.update_counter = 0;
	}

	this.update = function() {
		if (this.changed) {
			this.elem.style.display = "block";
			
			this.changed = false;
		}
		this.update_counter = this.update_counter + 1;
		if (this.update_counter == this.update_interval) {
			this.db.query_get("packetset/poll", this.on_poll_response);
			packetset.update_counter = 0;
		}
	}
}
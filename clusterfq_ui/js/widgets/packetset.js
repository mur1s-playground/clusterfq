var Packetset = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.update_interval = 1000;
	this.update_counter = 0;
	
	this.widget = new Widget("Packetset");
	
	this.elem = this.widget.elem;

	this.messagebox = new MessageBox("ps");
	this.messagebox.elem.className = "ps_messagebox";
	this.widget.content.appendChild(this.messagebox.elem);
	
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
		
		var state_num = 0;
		
		state_info_array.forEach(element => {
			update_identities_view = true;
			
			var p_state = packetset.packetset_info_states[element["packetset_state_info"]];
			var m_type = packetset.message_types[element["message_type"]];
			
			var removable_msg = false;
			
			if (p_state == "PS_OUT_COMPLETE" || p_state == "PS_IN_COMPLETE") removable_msg = true;
			
			var frames = 500 + 100 * state_num;
			
			var message_box_elem = document.createElement("div");
			
			//IDENTITY
			var message_box_identity_container = document.createElement("div");
			message_box_identity_container.className = "ps_messagebox_item_sub_container";
			
			var message_box_elem_identity_lbl = document.createElement("span");
			message_box_elem_identity_lbl.title = "Identity";
			message_box_elem_identity_lbl.appendChild(document.createTextNode("I"));
			message_box_elem_identity_lbl.className = "ps_messagebox_item_lbl ps_messagebox_item_identity_lbl";
			message_box_identity_container.appendChild(message_box_elem_identity_lbl);
			
			var message_box_elem_identity = document.createElement("span");
			message_box_elem_identity.appendChild(document.createTextNode(identities.identities_json["identities"][element["identity_id"]]["name"]));
			message_box_elem_identity.className = "ps_messagebox_item_content";
			message_box_identity_container.appendChild(message_box_elem_identity);
			
			message_box_elem.appendChild(message_box_identity_container);
				
			//CONTACT
			var message_box_contact_container = document.createElement("div");
			message_box_contact_container.className = "ps_messagebox_item_sub_container";
			
			var message_box_elem_contact_lbl = document.createElement("span");
			message_box_elem_contact_lbl.title = "Contact";
			message_box_elem_contact_lbl.appendChild(document.createTextNode("C"));
			message_box_elem_contact_lbl.className = "ps_messagebox_item_lbl ps_messagebox_item_contact_lbl";
			message_box_contact_container.appendChild(message_box_elem_contact_lbl);
			
			var message_box_elem_contact = document.createElement("span");
			message_box_elem_contact.appendChild(document.createTextNode(contacts.contacts_by_identity_id[element["identity_id"]]["contacts"][element["contact_id"]]["name"]));
			message_box_elem_contact.className = "ps_messagebox_item_content";
			message_box_contact_container.appendChild(message_box_elem_contact);
			
			message_box_elem.appendChild(message_box_contact_container);
			
			//STATE		
			var message_box_state_container = document.createElement("div");
			message_box_state_container.className = "ps_messagebox_item_sub_container";
			
			var message_box_elem_state_lbl = document.createElement("span");
			message_box_elem_state_lbl.title = "State";
			message_box_elem_state_lbl.appendChild(document.createTextNode("S"));
			message_box_elem_state_lbl.className = "ps_messagebox_item_lbl ps_messagebox_item_state_lbl";
			message_box_state_container.appendChild(message_box_elem_state_lbl);
			
			var message_box_elem_state = document.createElement("span");
			
			var state_arr = p_state.split("_");
			var state_txt = "";
			if (state_arr[1] == "IN") {
				state_txt = "\u2199";
			} else {
				state_txt = "\u2197";
			}
			if (state_arr[2] == "PENDING") {
				state_txt += " \u231b";
			} else if (state_arr[2] == "COMPLETE") {
				state_txt += " \u2713";
			} else { //CREATED
				state_txt += " \u25ef";
			}
			message_box_elem_state.appendChild(document.createTextNode(state_txt));
			message_box_elem_state.title = "Direction: " + state_arr[1] + ", Status: " + state_arr[2];
			message_box_elem_state.className = "ps_messagebox_item_content " + state_arr[2];
			message_box_state_container.appendChild(message_box_elem_state);
			
			message_box_elem.appendChild(message_box_state_container);
			
			//MESSAGE TYPE
			var message_box_msg_type_container = document.createElement("div");
			message_box_msg_type_container.className = "ps_messagebox_item_sub_container";
			
			var message_box_elem_msg_type_lbl = document.createElement("span");
			message_box_elem_msg_type_lbl.title = "Message Type";
			message_box_elem_msg_type_lbl.appendChild(document.createTextNode("M"));
			message_box_elem_msg_type_lbl.className = "ps_messagebox_item_lbl ps_messagebox_item_mt_lbl";
			message_box_msg_type_container.appendChild(message_box_elem_msg_type_lbl)
			
			var message_box_elem_msg_type = document.createElement("span");
			
			var mt_arr = m_type.split("_");
			var mt_text = "";
			if (mt_arr[1] == "ESTABLISH") {
				mt_text = "&#128279;";
			} else if (mt_arr[1] == "MIGRATE") {
				mt_text = "&#128472;";
			} else if (mt_arr[1] == "DROP") {
				mt_text = "&#128465;";
			} else if (mt_arr[1] == "MESSAGE") {
				mt_text = "\u2709";
			} else if (mt_arr[1] == "FILE") {
				mt_text = "&#128462;";
			} else if (mt_arr[1] == "RECEIPT") {
				mt_text = "&#129534;";
			} else if (mt_arr[1] == "UNKNOWN") {
				mt_text = "&#92228;";
			}
			if (mt_arr.length == 3) {
				if (mt_arr[2] == "CONTACT") {
					mt_text += " &#129485;";
				} else if (mt_arr[2] == "SESSION") {
					mt_text += " &#128272;";
				} else if (mt_arr[2] == "PUBKEY") {
					mt_text += " &#128273;";
				} else if (mt_arr[2] == "ADDRESS") {
					mt_text += " \u2316";
				} else if (mt_arr[2] == "COMPLETE") {
					mt_text += " \u2713";
				}
			}
			message_box_elem_msg_type.title = m_type;
			message_box_elem_msg_type.innerHTML = mt_text;
			message_box_elem_msg_type.className = "ps_messagebox_item_content";
			message_box_msg_type_container.appendChild(message_box_elem_msg_type);
			
			message_box_elem.appendChild(message_box_msg_type_container);
			
			packetset.messagebox.message_add(message_box_elem, frames, "ps_messagebox_item", element["hash_id"], removable_msg);

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
			state_num++;
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
			this.changed = false;
		}
		this.update_counter = this.update_counter + 1;
		if (this.update_counter == this.update_interval) {
			this.db.query_get("packetset/poll", this.on_poll_response);
			packetset.update_counter = 0;
		}
		this.messagebox.update();
	}
}
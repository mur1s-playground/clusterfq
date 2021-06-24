var Identities = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.identities_json = null;
	this.identity_selected_id = -1;
	
	this.get_name = function(identity_id) {
		if (identities.identities_json != null && identities.identities_json.hasOwnProperty("identities")) {
			for (var idx in identities.identities_json["identities"]) {
				if (identities.identities_json["identities"].hasOwnProperty(idx)) {
					if (identities.identities_json["identities"][idx]["id"] == identity_id) {
						return identities.identities_json["identities"][idx]["name"];
					}
				}
			}
		}
		return "";
	}
	
	this.widget = new Widget("Identities");

	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.menu = document.createElement("div");
	this.menu.className = "menu";

	this.add_btn = document.createElement("button");
	this.add_btn.id = this.widget.name + "_identity_add_btn";
	this.add_btn.widget_name = this.widget.name;
	this.add_btn.appendChild(document.createTextNode("+"));
	this.add_btn.onclick = function() {
		document.getElementById(this.widget_name + "_add_view").style.display = "block";
		this.style.display = "none";
	}
	
	this.packetset_toggle_btn = document.createElement("button");
	this.packetset_toggle_btn.style.display = "none";
	this.packetset_toggle_btn.id = this.widget.name + "_packetset_toggle_button";
	this.packetset_toggle_btn.innerHTML = "&#128295;";
	this.packetset_toggle_btn.title = "Show/Hide packetset";
	this.packetset_toggle_btn.onclick = function() {
		if (backend_element.style.display == "none") {
			backend_element.style.display = "block";
		} else {
			backend_element.style.display = "none";
		}
	}
	
	this.on_exit_response = function() {
		
	}
	
	this.exit_btn = document.createElement("button");
	this.exit_btn.obj = this;
	this.exit_btn.style.display = "none";
	this.exit_btn.id = this.widget.name + "_exit_button";
	this.exit_btn.innerHTML = "&#128682;";
	this.exit_btn.title = "Exit";
	this.exit_btn.onclick = function() {
		this.obj.db.query_post("exit", "{ }", identities.on_identity_share_response);
	}
	
	this.settings_button = document.createElement("button");
	this.settings_button.widget_name = this.widget.name;
	this.settings_button.appendChild(document.createTextNode("\u2630"));
	this.settings_button.onclick = function() {
		identities.toggle_secondary_controls();
	}
	
	this.widget.content.appendChild(this.menu);
	
	//Identity Add view
	this.add_view = document.createElement("div");
	this.add_view.id = this.widget.name + "_add_view";
	this.add_view.style.display = "none";
	this.add_view.className = "add_view";
		
	this.update_add_view = function() {
		this.add_view.innerHTML = "";
		
		this.add_name = document.createElement("input");
		this.add_name.id = this.widget.name + "_Name";
		this.add_name.type = "text";
		this.add_name.placeholder = "Name";
		this.add_view.appendChild(this.add_name);
		
		this.cancel_exec = document.createElement("button");
		this.cancel_exec.widget_name = this.widget.name;
		this.cancel_exec.appendChild(document.createTextNode("\u26cc"));
		this.cancel_exec.onclick = function() {
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
			document.getElementById(this.widget_name + "_identity_add_btn").style.display = "block";
		}
		this.add_view.appendChild(this.cancel_exec);
		
		this.add_exec = document.createElement("button");
		this.add_exec.obj = this;
		this.add_exec.widget_name = this.widget.name;
		this.add_exec.innerHTML = "&#10003;";
		
		this.add_exec.onclick = function() {
			this.obj.add();
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
			document.getElementById(this.widget_name + "_identity_add_btn").style.display = "block";
		}
		this.add_view.appendChild(this.add_exec);
	}
	
	this.menu.appendChild(this.add_view);
	this.menu.appendChild(this.add_btn);
	this.menu.appendChild(this.packetset_toggle_btn);
	this.menu.appendChild(this.exit_btn);
	this.menu.appendChild(this.settings_button);
	
	
	this.identities_view = document.createElement("div");
	this.identities_view.id = this.widget.name + "_identities_view";
	this.identities_view.style.display = "inline";
	
	this.share_info = null;
	
	//Share view
	this.share_view = document.createElement("div");
	this.share_view.id = this.widget.name + "_share_view";
	this.share_view.style.display = "none";
	this.share_view.className = "id_container";
	
	this.update_share_view = function() {
		this.share_view.innerHTML = "";
		
		this.add_name_s = document.createElement("input");
		this.add_name_s.id = this.widget.name + "_NameS";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_name_s.value = share_info_o["name"];
		}
		this.add_name_s.type = "text";
		this.add_name_s.placeholder = "Name";
		this.add_name_s.title = "Shared identity";
		this.share_view.appendChild(this.add_name_s);
		
		this.add_desc = document.createElement("textarea");
		this.add_desc.id = this.widget.name + "_Pubkey";
		this.add_desc.title = "Public Key";
		this.add_desc.rows = 1;
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_desc.innerHTML = share_info_o["pubkey"];
		}
		this.share_view.appendChild(this.add_desc);
		
		this.add_address = document.createElement("input");
		this.add_address.id = this.widget.name + "_Address";
		this.add_address.title = "Address";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_address.value = share_info_o["address"];
		}
		this.add_address.type = "text";
		this.add_address.placeholder = "Address";
		this.share_view.appendChild(this.add_address);
		
		this.add_exec = document.createElement("button");
		this.add_exec.widget_name = this.widget.name;
		this.add_exec.innerHTML = "&#10003";
		this.add_exec.title = "Close";
		this.add_exec.onclick = function() {
			document.getElementById(this.widget_name + "_share_view").style.display = "none";
		}
		this.share_view.appendChild(this.add_exec);
	}
	
	this.widget.content.appendChild(this.share_view);
	
	this.on_identity_share_response = function() {
		identities.share_info = JSON.parse(this.responseText);
		identities.update_share_view();
		document.getElementById("Identities_share_view").style.display = "flex";
		contacts.update_selected(identities.identity_selected_id);
	}
	
	this.on_migrate_key_response = function() {
		var resp = JSON.parse(this.responseText);
		document.getElementById(identities.widget.name + "_identity_migrate_key_" + resp["identity_id"]).disabled = false;
	}
	
	this.on_remove_obsolete_keys_response = function() {
		var resp = JSON.parse(this.responseText);
		document.getElementById(identities.widget.name + "_identity_remove_obsolete_keys_" + resp["identity_id"]).disabled = false;
	}
	
	this.on_identity_delete_response = function() {
		var resp = this.responseText;		
		identities.update_identities_list();
	}
	
	this.toggle_secondary_controls = function() {
		for (var k in identities.identities_json["identities"]) {
			if (identities.identities_json["identities"].hasOwnProperty(k)) {
				var migrate_key_button = document.getElementById(this.widget.name + "_identity_migrate_key_" + k);
				if (migrate_key_button.style.display == "none") {
					migrate_key_button.style.display = "inline";
				} else {
					migrate_key_button.style.display = "none";
				}
				var remove_obsolete_button = document.getElementById(this.widget.name + "_identity_remove_obsolete_keys_" + k);
				if (remove_obsolete_button.style.display == "none") {
					remove_obsolete_button.style.display = "inline";
				} else {
					remove_obsolete_button.style.display = "none";
				}
				var delete_button = document.getElementById(this.widget.name + "_identity_delete_" + k);
				if (delete_button.style.display == "none") {
					delete_button.style.display = "inline";
				} else {
					delete_button.style.display = "none";
				}
			}
		}
		if (this.packetset_toggle_btn.style.display == "none") {
			this.packetset_toggle_btn.style.display = "inline";
		} else {
			this.packetset_toggle_btn.style.display = "none";
		}
		if (this.exit_btn.style.display == "none") {
			this.exit_btn.style.display = "inline";
		} else {
			this.exit_btn.style.display = "none";
		}
	}
	
	this.update_identities_view = function() {
		this.identities_view.innerHTML = "";
		this.packetset_toggle_btn.style.display = "none";
		this.exit_btn.style.display = "none";
		
		if (identities.identities_json != null) {
			for (var k in identities.identities_json["identities"]) {
				if (!identities.identities_json["identities"].hasOwnProperty(k)) continue;
				var element = identities.identities_json["identities"][k];
				var identity = document.createElement("div");
				identity.id = this.widget.name + "_identity_" + element["id"];
				identity.className = "id_container";
				
				var name = document.createElement("span");
				name.appendChild(document.createTextNode(element["name"]));
				name.obj = this;
				name.identity_id = element["id"];
				name.className = "id_name";
				
				if (element["id"] == this.identity_selected_id) {
					name.className += " selected_i";
				}
				identity.appendChild(name);
				
				name.onclick = function() {
					this.obj.identity_selected_id = this.identity_id;
					contacts.update_selected(this.identity_id);
					this.obj.changed_f();
				};
				
				var received = document.createElement("span");
				received.id =  this.widget.name + "_identity_" + element["id"] + "_received";
				received.appendChild(document.createTextNode("\u2731"))
				received.style.visibility = "hidden";
				received.className = "status_msg_received";
				name.appendChild(received);

				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("new_msg") &&
					identities.identities_json["identities"][element["id"]]["new_msg"] === true) {
						if (element["id"] == this.identity_selected_id) {
							identities.identities_json["identities"][element["id"]]["new_msg"] = false;
						} else {
							received.style.visibility = "visible";
						}
				}

				var status_container = document.createElement("span");
				status_container.className = "container";
				identity.appendChild(status_container);
				
				if (this.identities_json != null) {
					status_container.appendChild(identities.identities_json["identities"][element["id"]]["status_msgbox"].elem);
					identities.identities_json["identities"][element["id"]]["status_msgbox"].elem.className = "statusbox";
				}
				
				var share_controls = document.createElement("div");
				share_controls.className = "id_controls";
				identity.appendChild(share_controls);
				
				var share_to_name = document.createElement("input");
				share_to_name.id = this.widget.name + "_identity_share_" + element["id"] + "_share_to_name";
				share_to_name.type = "text";
				share_to_name.placeholder = "Share to name";
				share_to_name.style.display = "none";
				share_controls.appendChild(share_to_name);
				
				var cancel_share_btn = document.createElement("button");
				cancel_share_btn.obj = this;
				cancel_share_btn.identity_id = element["id"];
				cancel_share_btn.id = this.widget.name + "_identity_share_" + element["id"] + "_cancel_share_btn";
				cancel_share_btn.style.display = "none";
				cancel_share_btn.appendChild(document.createTextNode("\u26cc"));
				cancel_share_btn.onclick = function() {
					var share_to_name = document.getElementById(this.obj.widget.name + "_identity_share_" + this.identity_id + "_share_to_name");
					share_to_name.style.display = "none";
					var cancel_share_btn = document.getElementById(this.obj.widget.name + "_identity_share_" + this.identity_id + "_cancel_share_btn");
					cancel_share_btn.style.display = "none";
					var share_to_name_btn = document.getElementById(this.obj.widget.name + "_identity_share_" + this.identity_id + "_share_to_name_btn");
					share_to_name_btn.style.display = "none";
					
					var identity_share = document.getElementById(this.obj.widget.name + "_identity_share_" + this.identity_id);
					identity_share.style.display = "inline";
				}
				share_controls.appendChild(cancel_share_btn);
				
				var share_to_name_btn = document.createElement("button");
				share_to_name_btn.id = this.widget.name + "_identity_share_" + element["id"] + "_share_to_name_btn";
				share_to_name_btn.obj = this;
				share_to_name_btn.identity_id = element["id"];
				share_to_name_btn.base_name = this.widget.name + "_identity_share_" + element["id"];
				share_to_name_btn.widget_name = this.widget.name;
				share_to_name_btn.innerHTML = "&#10003;";
				share_to_name_btn.style.display = "none";
				share_to_name_btn.onclick = function() {
					var share_to_name = document.getElementById(this.base_name + "_share_to_name");
					share_to_name.style.display = "none";
					
					var cancel_share_btn = document.getElementById(this.base_name + "_cancel_share_btn");
					cancel_share_btn.style.display = "none";
					
					this.style.display = "none";				
					this.obj.db.query_post("identity/share?identity_id=" + this.identity_id + "&name_to=" + document.getElementById(this.base_name + "_share_to_name").value, "{ }", identities.on_identity_share_response);
					var share_btn = document.getElementById(this.base_name);
					share_btn.style.display = "inline";
				}
				share_controls.appendChild(share_to_name_btn);
			
				var share_btn = document.createElement("button");
				share_btn.id = this.widget.name + "_identity_share_" + element["id"];
				share_btn.obj = this;
				share_btn.widget_name = this.widget.name;
				share_btn.appendChild(document.createTextNode("\u26ec"));
				share_btn.title = "Share";
				share_btn.onclick = function() {
					var share_to_name = document.getElementById(this.id + "_share_to_name");
					share_to_name.style.display = "inline";
					var cancel_share_btn = document.getElementById(this.id + "_cancel_share_btn");
					cancel_share_btn.style.display = "inline";
					var share_to_name_btn = document.getElementById(this.id + "_share_to_name_btn");
					share_to_name_btn.style.display = "inline";
					
					this.style.display = "none";
				}
				share_controls.appendChild(share_btn);
					
				var migrate_pubkey_btn = document.createElement("button");
				migrate_pubkey_btn.id = this.widget.name + "_identity_migrate_key_" + element["id"];
				migrate_pubkey_btn.obj = this;
				migrate_pubkey_btn.identity_id = element["id"];
				migrate_pubkey_btn.innerHTML = "&#128472;&#128273;";
				migrate_pubkey_btn.title = "Migrate key";
				migrate_pubkey_btn.style.display = "none";
				migrate_pubkey_btn.onclick = function() {
					this.disabled = true;
					this.obj.db.query_post("identity/migrate_key?identity_id=" + this.identity_id, "{ }", identities.on_migrate_key_response);
				}
				share_controls.appendChild(migrate_pubkey_btn);
				
				var remove_obsolete_btn = document.createElement("button");
				remove_obsolete_btn.id = this.widget.name + "_identity_remove_obsolete_keys_" + element["id"];
				remove_obsolete_btn.obj = this;
				remove_obsolete_btn.identity_id = element["id"];
				remove_obsolete_btn.innerHTML = "&#128465;&#128273;";
				remove_obsolete_btn.title = "Remove unused keys";
				remove_obsolete_btn.style.display = "none";
				remove_obsolete_btn.onclick = function() {
					this.disabled = true;
					this.obj.db.query_post("identity/remove_obsolete_keys?identity_id=" + this.identity_id, "{ }", identities.on_remove_obsolete_keys_response);
				}
				share_controls.appendChild(remove_obsolete_btn);
				
				var delete_identity = document.createElement("button");
				delete_identity.id = this.widget.name + "_identity_delete_" + element["id"];
				delete_identity.obj = this;
				delete_identity.identity_id = element["id"];
				delete_identity.identity_name = element["name"];
				delete_identity.innerHTML = "&#128465;";
				delete_identity.title = "Delete identity";
				delete_identity.style.display = "none";
				delete_identity.onclick = function() {
					this.disabled = true;
					var r = confirm("Delete identity \""+ this.identity_name +"\" and all contacts irreversibly?");
					if (r == true) {
						this.obj.db.query_post("identity/delete?identity_id=" + this.identity_id, "{ }", identities.on_identity_delete_response);
					} else {
						
					}
				}
				share_controls.appendChild(delete_identity);
				
				this.identities_view.appendChild(identity);
			};
		}
	}
	
	this.widget.content.appendChild(this.identities_view);
	
	this.changed = true;
	
	this.changed_f = function() {
		this.changed = true;
		if (this.change_dependencies != null) {
			for (var i = 0; i < this.change_dependencies.length; i++) {
				this.change_dependencies[i].changed_f();
			}
		}
	}

	this.on_identity_create_response = function() {
		identities.update_identities_list();
	}

	this.add = function() {
		var name = document.getElementById(identities.widget.name + "_Name").value;
		db.query_post("identity/create?name=" + name, "{ }", identities.on_identity_create_response);
	}
	
	this.identities_loaded = false;
	
	this.on_identities_list_response = function() {
		var json_res = JSON.parse(this.responseText);
		if (identities.identities_json == null) {
			identities.identities_json = {};
			identities.identities_json["identities"] = {};
		}
		for (var elem in json_res["identities"]) {
			if (json_res["identities"].hasOwnProperty(elem)) {
				if (!identities.identities_json["identities"].hasOwnProperty(json_res["identities"][elem]["id"])) {
					identities.identities_json["identities"][json_res["identities"][elem]["id"]] = json_res["identities"][elem];
					identities.identities_json["identities"][json_res["identities"][elem]["id"]]["status_msgbox"] = new MessageBox("status" + "_" + json_res["identities"][elem]["id"]);
				} else {
					for (var k in json_res["identities"][elem]) {
						if (json_res["identities"][elem].hasOwnProperty(k)) {
							identities.identities_json["identities"][json_res["identities"][elem]["id"]][k] = json_res["identities"][elem][k];
						}
					}
				}
			}
		}
		
		for (var elem in identities.identities_json["identities"]) {
			if (identities.identities_json["identities"].hasOwnProperty(elem)) {
				var id_elem = identities.identities_json["identities"][elem];
				var found = false;
				for (var ids = 0; ids < json_res["identities"].length; ids++) {
					if (json_res["identities"][ids]["id"] == id_elem["id"]) {
						found = true;
						break;
					}
				}
				if (!found) {
					delete contacts.contacts_by_identity_id[id_elem["id"]];
					delete identities.identities_json["identities"][id_elem["id"]];
					if (id_elem["id"] == identities.identity_selected_id) {
						identities.identity_selected_id = -1;
						contacts.update_selected(identities.identity_selected_id);
					}
				}
			}
		}	
		identities.changed_f();
	}
	
	this.update_identities_list = function() {
		db.query_get("identity/list", identities.on_identities_list_response);
		this.changed = true;
	}
	
	this.on_identities_load_response = function() {
		identities.update_identities_list();
	}

	this.update = function() {
		if (this.changed) {
			this.changed = false;
			this.elem.style.display = "block";
			
			this.update_add_view();
			
			if (!this.identities_loaded) {
				this.identities_loaded = true;
				this.db.query_post("identity/load_all", "{ }", identities.on_identities_load_response);
			}
			
			if (this.identities_json != null) {
				this.update_identities_view();
			}
		}
		if (identities.identities_json != null) {
			for (var k in identities.identities_json["identities"]) {
				if (!identities.identities_json["identities"].hasOwnProperty(k)) continue;
				var element = identities.identities_json["identities"][k];
				element["status_msgbox"].update();
			}
		}
	}
}
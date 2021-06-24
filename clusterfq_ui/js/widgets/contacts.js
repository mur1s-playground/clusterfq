var Contacts = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.selected_identity_id = -1;
	this.selected_contact_id = -1;
	this.contacts_by_identity_id = {};
	
	this.widget = new Widget("Contacts");
	
	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.menu = document.createElement("div");
	this.menu.className = "menu";

	this.add_btn = document.createElement("button");
	this.add_btn.id = this.widget.name + "_contact_add_btn";
	this.add_btn.widget_name = this.widget.name;
	this.add_btn.innerHTML = "+";
	this.add_btn.style.display = "block";
	this.add_btn.onclick = function() {
		document.getElementById(this.widget_name + "_add_view").style.display = "flex";
		this.style.display = "none";
	}
	
	this.menu.appendChild(this.add_btn);
	
	
	this.add_view = document.createElement("span");
	this.add_view.id = this.widget.name + "_add_view";
	this.add_view.className = "container_0";
	this.add_view.style.display = "none";
	
	this.update_add_view = function() {
		this.add_view.innerHTML = "";
		
		this.add_name = document.createElement("input");
		this.add_name.id = this.widget.name + "_Name";
		this.add_name.type = "text";
		this.add_name.placeholder = "Name";
		this.add_view.appendChild(this.add_name);
		
		this.add_view.innerHTML += "<br>";
		this.add_desc = document.createElement("textarea");
		this.add_desc.title = "Public Key";
		this.add_desc.rows = 1;
		this.add_desc.id = this.widget.name + "_Pubkey";
		this.add_view.appendChild(this.add_desc);
		
		this.add_address = document.createElement("input");
		this.add_address.id = this.widget.name + "_Address";
		this.add_address.type = "text";
		this.add_address.placeholder = "Address";
		this.add_view.appendChild(this.add_address);
		
		this.cancel_exec = document.createElement("button");
		this.cancel_exec.widget_name = this.widget.name;
		this.cancel_exec.appendChild(document.createTextNode("\u26cc"));
		this.cancel_exec.onclick = function() {
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
			document.getElementById(this.widget_name + "_contact_add_btn").style.display = "block";
		}
		this.add_view.appendChild(this.cancel_exec);
		
		
		this.add_exec = document.createElement("button");
		this.add_exec.obj = this;
		this.add_exec.widget_name = this.widget.name;
		this.add_exec.innerHTML = "&#10003;";
		this.add_exec.onclick = function() {
			this.obj.contact_add();
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
			document.getElementById(this.widget_name + "_contact_add_btn").style.display = "block";
		}
		this.add_view.appendChild(this.add_exec);
	}

	this.menu.appendChild(this.add_view);
	
	
	this.on_search_input_change = function() {
		if (this.contacts_view.childNodes) {
			if (this.contacts_view.childNodes.length > 1) {
				for (var k = 0; k < this.contacts_view.childNodes.length; k++) {
					if (this.contacts_view.childNodes[k].nodeName == "DIV") {
						if (this.contacts_view.childNodes[k].childNodes[0].innerHTML.includes(this.search_input.value)) {
							this.contacts_view.childNodes[k].style.display = "flex";
						} else {
							this.contacts_view.childNodes[k].style.display = "none";
						}
					}
				}
			}
		}
	}
	
	this.search_input = document.createElement("input");
	this.search_input.obj = this;
	this.search_input.id = this.widget_name + "_contact_search_input";
	this.search_input.style.display = "none";
	this.search_input.placeholder = "Search";
	this.search_input.onkeyup = function() {
		this.obj.on_search_input_change();
	}
	this.menu.appendChild(this.search_input);
	
	this.toggle_search_input = function() {
		if (this.search_input.style.display == "none") {
			this.search_input.style.display = "inline";
			this.search_btn.innerHTML = "x";
			this.search_btn.title = "Close contact filter";
		} else {
			this.search_input.style.display = "none";
			this.search_input.value = "";
			this.search_btn.innerHTML = "&#128269;";
			this.search_btn.title = "Open contact filter";
			this.on_search_input_change();
		}
	}
	
	this.search_btn = document.createElement("button");
	this.search_btn.id = this.widget.name + "_contact_search_btn";
	this.search_btn.obj = this;
	this.search_btn.innerHTML = "&#128269;";
	this.search_btn.title = "Open contact filter";
	this.search_btn.onclick = function() {
		this.obj.toggle_search_input();
	}
	this.menu.appendChild(this.search_btn);
	
	this.settings_button = document.createElement("button");
	this.settings_button.widget_name = this.widget.name;
	this.settings_button.appendChild(document.createTextNode("\u2630"));
	this.settings_button.onclick = function() {
		contacts.toggle_secondary_controls();
	}
	this.menu.appendChild(this.settings_button);
	
	this.toggle_secondary_controls = function() {
		if (contacts.contacts_by_identity_id.hasOwnProperty(contacts.selected_identity_id)) {
			for (var k in contacts.contacts_by_identity_id[contacts.selected_identity_id]["contacts"]) {
				if (contacts.contacts_by_identity_id[contacts.selected_identity_id]["contacts"].hasOwnProperty(k)) {
					 var delete_button = document.getElementById(contacts.widget.name + "_contact_delete_" + contacts.selected_identity_id + "_" + k);
					 if (delete_button.style.display = "none") {
						 delete_button.style.display = "inline";
					 } else {
						 delete_button.style.display = "none";
					 }
				}
			}
		}
	}

	this.widget.content.appendChild(this.menu);

	this.on_contact_add_response = function() {
		contacts.changed_f();
	}

	this.contact_add = function() {
		var name = document.getElementById(this.widget.name + "_Name").value;
		var pubkey = document.getElementById(this.widget.name + "_Pubkey").value;
		var address = document.getElementById(this.widget.name + "_Address").value;
		this.db.query_post("identity/contact_add?identity_id=" + this.selected_identity_id + "&name=" + name + "&address=" + address, pubkey, contacts.on_contact_add_response);
	}
		
	this.contacts_view = document.createElement("div");
	this.contacts_view.id = this.widget.name + "_contacts_view";
	this.contacts_view.className = "contacts_list";
	
	this.on_contact_delete_response = function() {
		contacts.changed_f();
	}

	this.update_contacts_view = function() {
		this.contacts_view.innerHTML = "";
		
		if (this.contacts_by_identity_id != null && this.selected_identity_id > -1) {
			for(var element in this.contacts_by_identity_id) {
				if (this.contacts_by_identity_id.hasOwnProperty(element)) {
					var el = this.contacts_by_identity_id[element];
					if (el["identity_id"] == this.selected_identity_id) {
					for (var i_id in el["contacts"]) {
						if (!el["contacts"].hasOwnProperty(i_id)) continue;
						var elem = el["contacts"][i_id];
						var contact = document.createElement("div");
						contact.obj = this;
						contact.identity_id = el["identity_id"];
						contact.contact_id = elem["id"];
						contact.className = "id_container";
						
						var name = document.createElement("span");
						name.innerHTML = elem["name"];
						name.className = "id_name";
						
						if (elem["id"] == this.selected_contact_id) {
							name.className += " selected_c";
						}

						contact.appendChild(name);
						
						contact.id = this.widget.name + "_contact_" + elem["id"];
						
						if (elem["address_available"] == 0) {
							name.className += " not_accessible_c";
						} else {
							contact.onclick = function() {
								this.obj.selected_contact_id = this.contact_id;
								chat.update_selected(this.identity_id, this.contact_id);
								this.obj.changed_f();
							};
						}
						
						var received = document.createElement("span");
						received.id = this.widget.name + "_contact_" + el["identity_id"] + "_" + elem["id"] + "_received";
						received.appendChild(document.createTextNode("\u2731"))
						received.style.visibility = "hidden";
						received.className = "status_msg_received";
						name.appendChild(received);

						if (contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id].hasOwnProperty("new_msg") &&
							contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["new_msg"] === true) {
								if (contact.contact_id == this.selected_contact_id) {
									contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["new_msg"] = false;
								} else {
									received.style.visibility = "visible";
								}
						}

						var status_container = document.createElement("span");
						status_container.className = "container";
						contact.appendChild(status_container);
						
						status_container.appendChild(contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["status_msgbox"].elem);
						contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["status_msgbox"].elem.className = "statusbox";
						
						this.space_fill = document.createElement("div");
						this.space_fill.className = "id_controls";
						
						var delete_contact = document.createElement("button");
						delete_contact.id = this.widget.name + "_contact_delete_" + el["identity_id"] + "_" + elem["id"];
						delete_contact.obj = this;
						delete_contact.identity_id = el["identity_id"];
						delete_contact.contact_id = elem["id"];
						delete_contact.contact_name = elem["name"];
						delete_contact.innerHTML = "&#128465;";
						delete_contact.title = "Delete contact";
						delete_contact.style.display = "none";
						delete_contact.onclick = function() {
							this.disabled = true;
							var r = confirm("Delete contact \""+ this.contact_name +"\" irreversibly?");
							if (r == true) {
								this.disabled = true;
								this.obj.db.query_post("identity/contact_delete?identity_id=" + this.identity_id + "&contact_id=" + this.contact_id, "{ }", contacts.on_contact_delete_response);
							} else {
							}
						}
						this.space_fill.appendChild(delete_contact);
						
						contact.appendChild(this.space_fill);
						
						
						this.contacts_view.appendChild(contact);
					};
					break;
					}
				}
			}
		}
	}
	
	this.widget.content.appendChild(this.contacts_view);

	this.changed = true;
	
	this.changed_f = function() {
		this.changed = true;
		if (this.change_dependencies != null) {
			for (var i = 0; i < this.change_dependencies.length; i++) {
				this.change_dependencies[i].changed_f();
			}
		}
	}
	
	this.update_selected = function(identity_id) {
		this.selected_identity_id = identity_id;
		this.selected_contact_id = -1;
		chat.update_selected(identity_id, -1);
		this.changed_f();
	}
	
	this.on_contact_list_receive = function() {
		var json_res = JSON.parse(this.responseText);

		if (!contacts.contacts_by_identity_id.hasOwnProperty(json_res["identity_id"])) {
			contacts.contacts_by_identity_id[json_res["identity_id"]] = {};
			contacts.contacts_by_identity_id[json_res["identity_id"]]["identity_id"] = json_res["identity_id"];
			contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"] = {};
		}

		json_res["contacts"].forEach(element => {
			var c_id = element["id"];
			if (!contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"].hasOwnProperty(c_id)) {
				contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][c_id] = element;
				contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][c_id]["status_msgbox"] = new MessageBox("status" + "_" + json_res["identity_id"] + "_" + c_id);
			} else {
				for (var prop in element) {
					if (element.hasOwnProperty(prop)) {
						contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][c_id][prop] = element[prop];
					}
				}
			}
		});
		
		var deletion = false;
		for (var elem in contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"]) {
			if (contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"].hasOwnProperty(elem)) {
				var contact_elem = contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][elem];
				var found = false;
				for (var ids = 0; ids < json_res["contacts"].length; ids++) {
					if (json_res["contacts"][ids]["id"] == contact_elem["id"]) {
						found = true;
						break;
					}
				}
				if (!found) {
					deletion = true;
					delete contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][contact_elem["id"]];
					if (contacts.selected_contact_id == contact_elem["id"]) {
						contacts.selected_contact_id = -1;
						chat.update_selected(json_res["identity_id"], -1);
					}
				}
			}
		}	
		
		if (deletion) {
			contacts.changed_f();
		} else {
			contacts.update_contacts_view();
		}
	}

	this.update = function() {
		if (this.changed) {
			this.elem.style.display = "block";
			
			this.update_add_view();
			
			if (identities.identities_json != null) {
				for (var i_id in identities.identities_json["identities"]) {
					if (!identities.identities_json["identities"].hasOwnProperty(i_id)) continue;
					var element = identities.identities_json["identities"][i_id];
					this.db.query_get("identity/contact_list?identity_id=" + element["id"], contacts.on_contact_list_receive);
				};
			}
			
			this.update_contacts_view();
			
			if (this.selected_identity_id == -1) {
				this.widget.elem.style.display = "none";
			} else {
				this.widget.elem.style.display = "block";
			}
			
			this.changed = false;
		}
		
		if (this.contacts_by_identity_id != null) {
			for(var element in this.contacts_by_identity_id) {
				if (this.contacts_by_identity_id.hasOwnProperty(element)) {
					var el = this.contacts_by_identity_id[element];
					for (var i_id in el["contacts"]) {
						if (!el["contacts"].hasOwnProperty(i_id)) continue;
						var elem = el["contacts"][i_id];
						elem["status_msgbox"].update();
					}
				}
			}
		}
	}
}
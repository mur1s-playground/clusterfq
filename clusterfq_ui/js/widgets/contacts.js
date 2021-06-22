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
						contact.onclick = function() {
							this.obj.selected_contact_id = this.contact_id;
							chat.update_selected(this.identity_id, this.contact_id);
							this.obj.changed_f();
						};
						
						var status_container = document.createElement("span");
						status_container.className = "container";
						contact.appendChild(status_container);
						
						/* change css */
						var sending = document.createElement("span");
						sending.backgroundColor = "#eeeeee";
						sending.id = this.widget.name + "_contact_" + el["identity_id"] + "_" + elem["id"] + "_sending";
						sending.appendChild(document.createTextNode("\u2197"));
						status_container.appendChild(sending);
				
						var receiving = document.createElement("span");
						receiving.backgroundColor = "#eeeeee";
						receiving.id = this.widget.name + "_contact_" + el["identity_id"] + "_" + elem["id"] + "_receiving";
						receiving.appendChild(document.createTextNode("\u2199"));
						status_container.appendChild(receiving);
				
						var received = document.createElement("span");
						received.backgroundColor = "#eeeeee";
						received.id = this.widget.name + "_contact_" + el["identity_id"] + "_" + elem["id"] + "_received";
						received.appendChild(document.createTextNode("\u2731"))
						received.style.visibility = "hidden";
						status_container.appendChild(received);

						if (contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id].hasOwnProperty("PS_OUT_PENDING") && 
							contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["PS_OUT_PENDING"] === true) {
								sending.style.backgroundColor = "#0000ff";
								sending.style.color = "#eeeeee";
						} else {
								sending.style.backgroundColor = "#eeeeee";
								sending.style.color = "#000000";
						}

						if (contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id].hasOwnProperty("PS_IN_PENDING") &&
							contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["PS_IN_PENDING"] === true) {
								receiving.style.backgroundColor = "#00ff00";
						} else {
								receiving.style.backgroundColor = "#eeeeee";
						}

						if (contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id].hasOwnProperty("PS_IN_COMPLETE") &&
							contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["PS_IN_COMPLETE"] === true) {
								if (contact.contact_id == this.selected_contact_id) {
									contacts.contacts_by_identity_id[contact.identity_id]["contacts"][contact.contact_id]["PS_IN_COMPLETE"] = false;
								} else {
									received.style.visibility = "visible";
								}
						}
						/* --------- */
						
						this.space_fill = document.createElement("div");
						this.space_fill.className = "id_controls adjustscroll";
						contact.appendChild(this.space_fill);
						
						
						this.contacts_view.appendChild(contact);
					};
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
			} else {
				for (var prop in element) {
					if (element.hasOwnProperty(prop)) {
						contacts.contacts_by_identity_id[json_res["identity_id"]]["contacts"][c_id][prop] = element[prop];
					}
				}
			}
		});
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
	}
}
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
	this.menu.style.display = "flex";
	this.menu.style.justifyContent = "space-between";
	this.menu.style.backgroundColor = "#cccccc";
	this.menu.style.borderRadius = "5px";
	this.menu.style.margin = "5px";
	this.menu.style.padding = "10px";
	this.menu.style.justifyContent = "flex-end";

	this.add_btn = document.createElement("button");
	this.add_btn.id = this.widget.name + "_identity_add_btn";
	this.add_btn.widget_name = this.widget.name;
	this.add_btn.appendChild(document.createTextNode("+"));
	this.add_btn.style.display = "inline";
	this.add_btn.onclick = function() {
		document.getElementById(this.widget_name + "_add_view").style.display = "block";
		this.style.display = "none";
	}
	this.add_btn.style.borderRadius = "5px";
	this.add_btn.style.marginRight = "5px";
	this.add_btn.style.padding = "5px";
	
	this.settings_button = document.createElement("button");
	this.settings_button.widget_name = this.widget.name;
	this.settings_button.appendChild(document.createTextNode("\u2630"));
	this.settings_button.style.borderRadius = "5px";
	this.settings_button.style.padding = "5px";
	this.settings_button.style.display = "inline";
	
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
		this.add_name.style.borderRadius = "5px";
		this.add_name.style.marginRight = "5px";
		this.add_name.style.padding = "5px";
		this.add_view.appendChild(this.add_name);
		
		this.cancel_exec = document.createElement("button");
		this.cancel_exec.widget_name = this.widget.name;
		this.cancel_exec.appendChild(document.createTextNode("\u26cc"));
		this.cancel_exec.style.borderRadius = "5px";
		this.cancel_exec.style.marginRight = "5px";
		this.cancel_exec.style.padding = "5px";
		this.cancel_exec.onclick = function() {
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
			document.getElementById(this.widget_name + "_identity_add_btn").style.display = "block";
		}
		this.add_view.appendChild(this.cancel_exec);
		
		this.add_exec = document.createElement("button");
		this.add_exec.style.borderRadius = "5px";
		this.add_exec.style.marginRight = "5px";
		this.add_exec.style.padding = "5px";
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
	this.menu.appendChild(this.settings_button);
	
	
	this.identities_view = document.createElement("div");
	this.identities_view.id = this.widget.name + "_identities_view";
	this.identities_view.style.display = "inline";
	
	this.share_info = null;
	
	//Share view
	this.share_view = document.createElement("div");
	this.share_view.id = this.widget.name + "_share_view";
	this.share_view.style.display = "none";
	this.share_view.className = "share_view";
	this.share_view.padding = "10px";
	this.share_view.margin = "5px";
	this.share_view.backgroundColor = "#cccccc";
	this.share_view.borderRadius = "5px";
	
	this.update_share_view = function() {
		this.share_view.innerHTML = "";
		
		this.add_name_s = document.createElement("input");
		this.add_name_s.id = this.widget.name + "_NameS";
		this.add_name_s.style.borderRadius = "5px";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_name_s.value = share_info_o["name"];
		}
		this.add_name_s.type = "text";
		this.add_name_s.placeholder = "Name";
		this.add_name_s.title = "Shared identity";
		this.share_view.appendChild(this.add_name_s);
		
		this.add_desc = document.createElement("textarea");
		this.add_desc.style.borderRadius = "5px";
		this.add_desc.id = this.widget.name + "_Pubkey";
		this.add_desc.title = "Public Key";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_desc.innerHTML = share_info_o["pubkey"];
		}
		this.share_view.appendChild(this.add_desc);
		
		this.add_address = document.createElement("input");
		this.add_address.style.borderRadius = "5px";
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
		this.add_exec.style.borderRadius = "5px";
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
	}
	
	this.update_identities_view = function() {
		this.identities_view.innerHTML = "";
		
		if (identities.identities_json != null) {
			for (var k in identities.identities_json["identities"]) {
				if (!identities.identities_json["identities"].hasOwnProperty(k)) continue;
				var element = identities.identities_json["identities"][k];
				var identity = document.createElement("div");
				identity.id = this.widget.name + "_identity_" + element["id"];
				identity.style.display = "flex";
				identity.style.justifyContent = "space-between";
				identity.style.backgroundColor = "#cccccc";
				identity.style.borderRadius = "5px";
				identity.style.margin = "5px";
				identity.style.padding = "10px";
				
				var name = document.createElement("span");
				name.appendChild(document.createTextNode(element["name"]));
				name.obj = this;
				name.identity_id = element["id"];
				name.style.width = "50%";
				name.style.textAlign = "center";
				name.style.borderRadius = "5px";
				name.style.padding = "5px";
				name.style.margin = "5px";
				
				if (element["id"] == this.identity_selected_id) {
					name.style.backgroundColor = "#0000ff";
					name.style.color = "#ffffff";
				} else {
					name.style.backgroundColor = "#ffffff";
					name.style.color = "#000000";
				}
				identity.appendChild(name);
				
				name.onclick = function() {
					this.obj.identity_selected_id = this.identity_id;
					contacts.update_selected(this.identity_id);
					this.obj.changed_f();
				};
				
				var status_container = document.createElement("span");
				status_container.style.display = "inline-flex";
				identity.appendChild(status_container);
				
				var sending = document.createElement("span");
				sending.style.padding = "5px";
				sending.style.margin = "5px";
				sending.style.borderRadius = "5px";
				sending.id = this.widget.name + "_identity_" + element["id"] + "_sending";
				sending.appendChild(document.createTextNode("\u2197"));
				sending.backgroundColor = "#eeeeee";
				status_container.appendChild(sending);
				
				var receiving = document.createElement("span");
				receiving.style.padding = "5px";
				receiving.style.margin = "5px";
				receiving.style.borderRadius = "5px";
				receiving.id = this.widget.name + "_identity_" + element["id"] + "_receiving";
				receiving.appendChild(document.createTextNode("\u2199"));
				receiving.backgroundColor = "#eeeeee";
				status_container.appendChild(receiving);
				
				var received = document.createElement("span");
				received.style.padding = "5px";
				received.style.margin = "5px";
				received.style.borderRadius = "5px";
				received.id =  this.widget.name + "_identity_" + element["id"] + "_received";
				received.appendChild(document.createTextNode("\u2731"))
				received.style.visibility = "hidden";
				received.backgroundColor = "#eeeeee";
				status_container.appendChild(received);
				
				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_OUT_PENDING") &&
					identities.identities_json["identities"][element["id"]]["PS_OUT_PENDING"] === true) {
						sending.style.backgroundColor = "#0000ff";
						sending.style.color = "#ffffff";
				} else {
						sending.style.backgroundColor = "#eeeeee";
						sending.style.color = "#000000";
				}
				
				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_IN_PENDING") &&
					identities.identities_json["identities"][element["id"]]["PS_IN_PENDING"] === true) {
						receiving.style.backgroundColor = "#00ff00";
				} else {
						receiving.style.backgroundColor = "#eeeeee";
				}

				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_IN_COMPLETE") &&
					identities.identities_json["identities"][element["id"]]["PS_IN_COMPLETE"] === true) {
						if (element["id"] == this.identity_selected_id) {
							identities.identities_json["identities"][element["id"]]["PS_IN_COMPLETE"] = false;
						} else {
							received.style.visibility = "visible";
						}
				}
				
				var share_controls = document.createElement("div");
				share_controls.style.width = "30%";
				share_controls.style.display = "flex";
				share_controls.style.justifyContent = "flex-end";
				identity.appendChild(share_controls);
				
				var share_to_name = document.createElement("input");
				share_to_name.id = this.widget.name + "_identity_share_" + element["id"] + "_share_to_name";
				share_to_name.type = "text";
				share_to_name.placeholder = "Share to name";
				share_to_name.style.display = "none";
				share_to_name.style.borderRadius = "5px";
				share_to_name.style.marginRight = "5px";
				share_controls.appendChild(share_to_name);
				
				var cancel_share_btn = document.createElement("button");
				cancel_share_btn.obj = this;
				cancel_share_btn.identity_id = element["id"];
				cancel_share_btn.id = this.widget.name + "_identity_share_" + element["id"] + "_cancel_share_btn";
				cancel_share_btn.style.display = "none";
				cancel_share_btn.appendChild(document.createTextNode("\u26cc"));
				cancel_share_btn.style.borderRadius = "5px";
				cancel_share_btn.style.marginRight = "5px";
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
				share_to_name_btn.style.borderRadius = "5px";
				share_to_name_btn.onclick = function() {
					var share_to_name = document.getElementById(this.base_name + "_share_to_name");
					share_to_name.style.display = "none";
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
				share_btn.style.display = "inline";
				share_btn.style.borderRadius = "5px";
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
				
				this.identities_view.appendChild(identity);
			};
		}
		
		var hr = document.createElement("hr");
		this.identities_view.appendChild(hr);
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
				} else {
					for (var k in json_res["identities"][elem]) {
						if (json_res["identities"][elem].hasOwnProperty(k)) {
							identities.identities_json["identities"][json_res["identities"][elem]["id"]][k] = json_res["identities"][elem][k];
						}
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
	}
}
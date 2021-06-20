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

	this.add_btn = document.createElement("button");
	this.add_btn.widget_name = this.widget.name;
	this.add_btn.innerHTML = "+";
	this.add_btn.onclick = function() {
		document.getElementById(this.widget_name + "_add_view").style.display = "block";
	}
	this.menu.appendChild(this.add_btn);
	
	this.widget.content.appendChild(this.menu);
	
	//Identity Add view
	this.add_view = document.createElement("div");
	this.add_view.id = this.widget.name + "_add_view";
	this.add_view.style.display = "none";
	this.add_view.className = "add_view";
		
	this.update_add_view = function() {
		this.add_view.innerHTML = "<hr>";
		
		this.add_name = document.createElement("input");
		this.add_name.id = this.widget.name + "_Name";
		this.add_name.type = "text";
		this.add_name.placeholder = "Name";
		this.add_view.appendChild(this.add_name);
		
		this.cancel_exec = document.createElement("button");
		this.cancel_exec.widget_name = this.widget.name;
		this.cancel_exec.innerHTML = "x";
		this.cancel_exec.onclick = function() {
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
		}
		this.add_view.appendChild(this.cancel_exec);
		
		
		this.add_exec = document.createElement("button");
		this.add_exec.obj = this;
		this.add_exec.widget_name = this.widget.name;
		this.add_exec.innerHTML = "&#10003;";
		this.add_exec.onclick = function() {
			this.obj.add();
			document.getElementById(this.widget_name + "_add_view").style.display = "none";
		}
		this.add_view.appendChild(this.add_exec);
		
		var hr = document.createElement("hr");
		this.add_view.appendChild(hr);
	}

	this.widget.content.appendChild(this.add_view);
	
	this.identities_view = document.createElement("div");
	this.identities_view.id = this.widget.name + "_identities_view";
	this.identities_view.style.display = "inline";
	
	this.share_info = null;
	
	//Share view
	this.share_view = document.createElement("div");
	this.share_view.id = this.widget.name + "_share_view";
	this.share_view.style.display = "none";
	this.share_view.className = "share_view";
	
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
		this.share_view.appendChild(this.add_name_s);
		
		var label_dsc = document.createElement("label");
		label_dsc.innerHTML = "Public key";
		this.share_view.appendChild(label_dsc);
		this.add_desc = document.createElement("textarea");
		this.add_desc.id = this.widget.name + "_Pubkey";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_desc.innerHTML = share_info_o["pubkey"];
		}
		this.share_view.appendChild(this.add_desc);
		
		this.add_address = document.createElement("input");
		this.add_address.id = this.widget.name + "_Address";
		if (identities.share_info != null) {
			var share_info_o = identities.share_info["identity_share"];
			this.add_address.value = share_info_o["address"];
		}
		this.add_address.type = "text";
		this.add_address.placeholder = "Address";
		this.share_view.appendChild(this.add_address);
		
		this.add_exec = document.createElement("button");
		this.add_exec.widget_name = this.widget.name;
		this.add_exec.innerHTML = "&#10003;";
		this.add_exec.onclick = function() {
			document.getElementById(this.widget_name + "_share_view").style.display = "none";
		}
		this.share_view.appendChild(this.add_exec);
	}
	
	this.widget.content.appendChild(this.share_view);
	
	this.on_identity_share_response = function() {
		identities.share_info = JSON.parse(this.responseText);
		identities.update_share_view();
		document.getElementById("Identities_share_view").style.display = "block";
	}
	
	this.update_identities_view = function() {
		this.identities_view.innerHTML = "";
		
		if (identities.identities_json != null) {
			for (var k in identities.identities_json["identities"]) {
				if (!identities.identities_json["identities"].hasOwnProperty(k)) continue;
				var element = identities.identities_json["identities"][k];
				var identity = document.createElement("div");
				identity.id = this.widget.name + "_identity_" + element["id"];
				identity.obj = this;
				identity.identity_id = element["id"];
				identity.style.display = "flex";
				identity.style.justifyContent = "space-between";
				
				var name = document.createElement("span");
				name.innerHTML = element["name"];
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
				
				identity.onclick = function() {
					this.obj.identity_selected_id = this.identity_id;
					contacts.update_selected(this.identity_id);
					this.obj.changed_f();
				};
				
				var sending = document.createElement("span");
				sending.padding = "5px";
				sending.margin = "5px";
				sending.id = this.widget.name + "_identity_" + element["id"] + "_sending";
				sending.appendChild(document.createTextNode("\u2197"));
				identity.appendChild(sending);
				
				var receiving = document.createElement("span");
				receiving.padding = "5px";
				receiving.margin = "5px";
				receiving.id = this.widget.name + "_identity_" + element["id"] + "_receiving";
				receiving.appendChild(document.createTextNode("\u2199"));
				identity.appendChild(receiving);
				
				var received = document.createElement("span");
				received.padding = "5px";
				received.margin = "5px";
				received.id =  this.widget.name + "_identity_" + element["id"] + "_received";
				received.appendChild(document.createTextNode("\u2731"))
				received.style.visibility = "hidden";
				identity.appendChild(received);
				
				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_OUT_PENDING") &&
					identities.identities_json["identities"][element["id"]]["PS_OUT_PENDING"] === true) {
						sending.style.backgroundColor = "#0000ff";
						sending.style.color = "#ffffff";
				} else {
						sending.style.backgroundColor = "#ffffff";
						sending.style.color = "#000000";
				}
				
				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_IN_PENDING") &&
					identities.identities_json["identities"][element["id"]]["PS_IN_PENDING"] === true) {
						receiving.style.backgroundColor = "#00ff00";
				} else {
						receiving.style.backgroundColor = "#ffffff";
				}

				if (identities.identities_json["identities"][element["id"]].hasOwnProperty("PS_IN_COMPLETE") &&
					identities.identities_json["identities"][element["id"]]["PS_IN_COMPLETE"] === true) {
						if (element["id"] == this.identity_selected_id) {
							identities.identities_json["identities"][element["id"]]["PS_IN_COMPLETE"] = false;
						} else {
							received.style.visibility = "visible";
						}
				}
				
				var share_to_name = document.createElement("input");
				share_to_name.id = this.widget.name + "_identity_share_" + element["id"] + "_share_to_name";
				share_to_name.type = "text";
				share_to_name.placeholder = "Share to name";
				share_to_name.style.display = "none";
				identity.appendChild(share_to_name);
				
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
					this.style.display = "none";
					this.obj.db.query_post("identity/share?identity_id=" + this.identity_id + "&name_to=" + document.getElementById(this.base_name + "_share_to_name").value, "{ }", identities.on_identity_share_response);
					var share_btn = document.getElementById(this.base_name);
					share_btn.style.display = "inline";
				}
				identity.appendChild(share_to_name_btn);
			
				var share_btn = document.createElement("button");
				share_btn.id = this.widget.name + "_identity_share_" + element["id"];
				share_btn.obj = this;
				share_btn.widget_name = this.widget.name;
				share_btn.innerHTML = "share";
				share_btn.style.display = "inline";
				share_btn.onclick = function() {
					var share_to_name = document.getElementById(this.id + "_share_to_name");
					share_to_name.style.display = "inline";
					var share_to_name_btn = document.getElementById(this.id + "_share_to_name_btn");
					share_to_name_btn.style.display = "inline";
					this.style.display = "none";
				}
				identity.appendChild(share_btn);
				
				
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
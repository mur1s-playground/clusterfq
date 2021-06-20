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
	
	//Routine Menu
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
		//this.add_exec.obj = this;
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
			identities.identities_json["identities"].forEach(element => {
				var identity = document.createElement("div");
				identity.id = this.widget.name + "_identity_" + element["id"];
				identity.obj = this;
				identity.identity_id = element["id"];
				identity.innerHTML = element["name"];
				identity.style.display = "inline";
				if (element["id"] == this.identity_selected_id) {
					identity.style.backgroundColor = "#0000ff";
					identity.style.color = "#ffffff";
				} else {
					identity.style.backgroundColor = "#ffffff";
					identity.style.color = "#000000";
				}
				identity.onclick = function() {
					this.obj.identity_selected_id = this.identity_id;
					contacts.update_selected(this.identity_id);
					this.obj.changed_f();
				};
				
				this.identities_view.appendChild(identity);
				
				var share_to_name = document.createElement("input");
				share_to_name.id = this.widget.name + "_identity_share_" + element["id"] + "_share_to_name";
				share_to_name.type = "text";
				share_to_name.placeholder = "Share to name";
				share_to_name.style.display = "none";
				this.identities_view.appendChild(share_to_name);
				
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
				this.identities_view.appendChild(share_to_name_btn);
			
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
				this.identities_view.appendChild(share_btn);
				var br = document.createElement("br");
				this.identities_view.appendChild(br);
			});
		}
		
		var hr = document.createElement("hr");
		this.identities_view.appendChild(hr);
	}
	
	
	this.widget.content.appendChild(this.identities_view);
	
	//Routine List
	//this.routine_list = document.createElement("div");
	
	//this.widget.content.appendChild(this.routine_list);

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
		identities.identities_json = JSON.parse(this.responseText);
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
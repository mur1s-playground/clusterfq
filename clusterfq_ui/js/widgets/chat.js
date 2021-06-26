var Chat = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.identity_id = -1;
	this.identity_name = "";
	this.contact_id = -1;
	
	this.update_interval = 1000;
	this.update_counter = 0;
	
	this.chat_by_identity_and_contact_id = {};
	
	this.widget = new Widget("Chat");

	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.changed = true;
	
	this.chat = document.createElement("div");
	this.chat.id = this.widget.name + "_chat";
	
	this.widget.content.appendChild(messages.message_send_container);
	this.widget.content.appendChild(this.chat);

	this.chat_footer = document.createElement("div");
	this.chat_footer.className = "id_container_footer";
	this.widget.content.appendChild(this.chat_footer);
	
	this.update_selected = function(identity_id, contact_id) {
		chat.identity_id = identity_id;
		chat.contact_id = contact_id;
		chat.update_counter = chat.update_interval - 1;
		if (chat.identity_id > -1) {
			chat.identity_name = identities.get_name(identity_id);
		} else {
			chat.identity_name = "";
		}
		messages.update_selected(identity_id, contact_id);
	}
	
	this.changed_f = function() {
		this.changed = true;
		if (this.change_dependencies != null) {
			for (var i = 0; i < this.change_dependencies.length; i++) {
				this.change_dependencies[i].changed_f();
			}
		}
	}
	
	this.update_chat_msg = function(resp, prop) {
		var elem = document.getElementById(chat.widget.name + "_chat_" + prop);
		if (elem == undefined) {
			var source_is_identity = resp["chat"][prop]["sender"] == chat.identity_name;
			
			elem = document.createElement("div");
			elem.className = "chat";
			elem.id = chat.widget.name + "_chat_" + prop;
			
			var datetime = document.createElement("span")
			datetime.id = elem.id + "_datetime";
			datetime.className = "datetime";
			
			var pending = document.createElement("span");
			pending.id = elem.id + "_pending";
			if (resp["chat"][prop]["pending"] == 1) {
				pending.innerHTML = "\u231b";
				datetime.id = elem.id + "_datetime_pending";
				datetime.className += " pending";
				pending.className = "datetime pm_0";
			}
							
			var date = new Date(resp["chat"][prop]["time"] * 1000);
			
			var datetime_txt = null;
			if (navigator.language === undefined) {
				datetime_txt = document.createTextNode(date.toUTCString());
			} else {
				datetime_txt = document.createTextNode(date.toLocaleString(navigator.language));
			}
			
			datetime.appendChild(datetime_txt);
			
			var sender = document.createElement("span");
			sender.id = elem.id + "_sender";
			var sender_txt = document.createTextNode(resp["chat"][prop]["sender"]);
			sender.appendChild(sender_txt);
			
			
			var msg = document.createElement("span");
			msg.id = elem.id + "_msg";
			var msg_txt = null;
			if (resp["chat"][prop].hasOwnProperty("message")) {
				msg_txt = document.createTextNode(resp["chat"][prop]["message"]);
				msg.appendChild(msg_txt);
			} else if (resp["chat"][prop].hasOwnProperty("file")){
				var fname = resp["chat"][prop]["file"];
				var ext_a = fname.split(".");
				var ext = ext_a[ext_a.length - 1];
				if (ext == "pending") {
					ext = ext_a[ext_a.length - 2];
				}
				if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "gif") {
					msg.style.textAlign = "center";
					var ar = document.createElement("a");
					ar.href = resp["chat"][prop]["file"];
					ar.target = "_new";
					var img_f = document.createElement("img");
					img_f.src = resp["chat"][prop]["file"];
					img_f.className = "chat";
					ar.appendChild(img_f);
					
					msg.appendChild(ar);
				} else {
					msg_txt = document.createTextNode(resp["chat"][prop]["file"]);
					msg.appendChild(msg_txt);
				}
			}

			if (source_is_identity) {
				elem.className += " chat_out";
				
				sender.className = "selected_i";
				
				if (resp["chat"][prop]["pending"] == 1) {
					datetime.appendChild(document.createElement("br"));
					datetime.appendChild(pending);
				}
				
				elem.appendChild(datetime);
				elem.appendChild(sender);
				elem.appendChild(msg);
			} else {
				elem.className += " chat_in";
				
				sender.className = "selected_c";
				
				elem.appendChild(msg);
				elem.appendChild(sender);
				elem.appendChild(datetime);
			}
			chat.chat.prepend(elem);
		} else {
			if (resp["chat"][prop]["pending"] == 0) {
				var delivered = document.getElementById(chat.widget.name + "_chat_" + prop + "_datetime");
				if (delivered == undefined) {
					var pending = document.getElementById(chat.widget.name + "_chat_" + prop + "_pending");
					pending.id = chat.widget.name + "_chat_" + prop + "_datetime";
					
					var date = new Date(resp["chat"][prop]["time"] * 1000);
			
					var datetime_txt = null;
					if (navigator.language === undefined) {
						datetime_txt = document.createTextNode(date.toUTCString());
					} else {
						datetime_txt = document.createTextNode(date.toLocaleString(navigator.language));
					}
					pending.innerHTML = "";
					pending.appendChild(datetime_txt);
				}
			}
		}
	}
	
	this.on_chat_response = function() {
		var resp = JSON.parse(this.responseText);
		var most_recent = 0;
		
		if (!resp.hasOwnProperty("identity_id")) return;
		
		for (var prop in chat.chat_by_identity_and_contact_id[resp["identity_id"]][resp["contact_id"]]["chat"]) {
			var rsp = chat.chat_by_identity_and_contact_id[resp["identity_id"]][resp["contact_id"]];
			chat.update_chat_msg(rsp, prop);
		}
		
		for (var prop in resp["chat"]) {
			if (resp["chat"].hasOwnProperty(prop)) {
				if (resp["chat"][prop]["time"] > most_recent) {
					most_recent = resp["chat"][prop]["time"];
				}
				
				chat.chat_by_identity_and_contact_id[resp["identity_id"]][resp["contact_id"]]["chat"][prop] = resp["chat"][prop];
				chat.update_chat_msg(resp, prop);
			}
		}
		
		chat.chat_by_identity_and_contact_id[resp["identity_id"]][resp["contact_id"]]["most_recent_time"] = most_recent;
		chat.update_counter = 0;
	}

	this.update = function() {
		if (this.changed) {
			this.elem.style.display = "block";
			
			this.chat.innerHTML = "";
						
			this.update_counter = this.update_interval - 1;
			this.changed = false;

			if (this.identity_id == -1 || this.contact_id == -1) {
				this.widget.elem.style.display = "none";
			} else {
				this.widget.elem.style.display = "block";
			}
		}
		this.update_counter = this.update_counter + 1;
		if (this.update_counter == this.update_interval) {
			if (this.identity_id >= 0 && this.contact_id >= 0) {
				var most_recent = 0;
				if (chat.chat_by_identity_and_contact_id.hasOwnProperty(this.identity_id)) {
					if (chat.chat_by_identity_and_contact_id[this.identity_id].hasOwnProperty(this.contact_id)) {
						most_recent = chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id]["most_recent_time"];
					} else {
						chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id] = {};
						chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id]["most_recent_time"] = 0;
						chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id]["chat"] = {};
					}
				} else {
					chat.chat_by_identity_and_contact_id[this.identity_id] = {};
					chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id] = {};
					chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id]["most_recent_time"] = 0;
					chat.chat_by_identity_and_contact_id[this.identity_id][this.contact_id]["chat"] = {};
				}
				this.db.query_get("contact/chat?identity_id=" + this.identity_id + "&contact_id=" + this.contact_id + "&time_start=" + most_recent + "&time_end=0", this.on_chat_response);
			}
			chat.update_counter = 0;
		}
	}
}
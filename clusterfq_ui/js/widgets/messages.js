var Messages = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.identity_id = -1;
	this.contact_id = -1;
	
	this.widget = new Widget("Messages");

	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.changed = true;
	
	this.message_send_container = document.createElement("div");
	this.message_send_container.className = "menu";
	
	this.message_type = document.createElement("select");
	this.message_type.className = "message_type";
	this.message_type.obj = this;
	this.message_type.id = this.widget.name + "_type_select";
	this.message_type.onchange = function() {
		if (this.selectedIndex == 0) {
			document.getElementById(this.obj.widget.name + "_file_search").style.display = "none";
			document.getElementById(this.obj.widget.name + "_input").style.display = "block";
		} else {
			document.getElementById(this.obj.widget.name + "_input").style.display = "none";
			document.getElementById(this.obj.widget.name + "_file_search").style.display = "block";
		}
	}
	
	this.message_type_option_text = document.createElement("option");	
	this.message_type_option_text.appendChild(document.createTextNode("Text"));
	this.message_type.appendChild(this.message_type_option_text);
	
	this.message_type_option_file = document.createElement("option");
	this.message_type_option_file.appendChild(document.createTextNode("File"));
	this.message_type.appendChild(this.message_type_option_file);
	
	this.message_send_container.appendChild(this.message_type);
		
	this.on_message_send_initiated = function() {
		document.getElementById(messages.widget.name + "_send_button").disabled = false;
		var file_select = document.getElementById(messages.widget.name + "_file_search");
		file_select.value = null;
		file_select.disabled = false;
	}
	
	this.message_input = document.createElement("input");
	this.message_input.className = "message";
	this.message_input.obj = this;
	this.message_input.id = this.widget.name + "_input";
	
	this.message_input.onkeydown = function() {
		if (event.key === 'Enter') {
			var type = this.obj.message_type.options[this.obj.message_type.selectedIndex];
			
			if (type.innerHTML == "Text") {
				if (this.value.length > 0) {
					var input_text = this.value;
					this.value = "";
				
					this.obj.db.query_post("message/send?identity_id=" + this.obj.identity_id + "&contact_id=" + this.obj.contact_id + "&type=text", input_text, this.obj.on_message_send_initiated);
				}
			}
		}
	}
	
	this.message_send_container.appendChild(this.message_input);
	
	this.read_file = function() {
		var input = document.getElementById(this.widget.name + "_file_search");
		var file = input.files[0];
		var fr = new FileReader();
		fr.caller_ref = this;
		fr.onload = function(e) {
			let res = btoa(e.target.result);
			db.query_post("message/send?identity_id=" + messages.identity_id + "&contact_id=" + messages.contact_id + "&type=file&filename=" + input.value, res, messages.on_message_send_initiated);
		}
		fr.readAsBinaryString(file);
	}
	
	this.file_search_btn = document.createElement("input");
	this.file_search_btn.id = this.widget.name + "_file_search";
	this.file_search_btn.className = "message";
	this.file_search_btn.style.display = "none";
	this.file_search_btn.type = "file";
	
	this.message_send_container.appendChild(this.file_search_btn);
	
	this.message_send_btn = document.createElement("button");
	this.message_send_btn.id = this.widget.name + "_send_button";
	this.message_send_btn.obj = this;
	this.message_send_btn.innerHTML = "&#10003";
	this.message_send_btn.onclick = function() {
		var input_f = document.getElementById(this.obj.widget.name + "_input");
		var input_text = input_f.value;
		
		var type = this.obj.message_type.options[this.obj.message_type.selectedIndex];
			
		if (type.innerHTML == "Text") {
			if (this.value.length > 0) {
				var input_text = this.value;
				this.value = "";
			
				this.obj.db.query_post("message/send?identity_id=" + this.obj.identity_id + "&contact_id=" + this.obj.contact_id + "&type=text", input_text, this.obj.on_message_send_initiated);
			}
		} else if (type.innerHTML == "File") {
			var input = document.getElementById(messages.widget.name + "_file_search");
			if (input.value != null) {
				this.disabled = true;
				input.disabled = true;
				this.obj.read_file();
			}
		}
	}
	
	this.message_send_container.appendChild(this.message_send_btn);
	
	this.widget.content.appendChild(this.message_send_container);
	
	this.update_selected = function(identity_id, contact_id) {
		messages.identity_id = identity_id;
		messages.contact_id = contact_id;
	}
	
	this.changed_f = function() {
		this.changed = true;
		if (this.change_dependencies != null) {
			for (var i = 0; i < this.change_dependencies.length; i++) {
				this.change_dependencies[i].changed_f();
			}
		}
	}

	this.update = function() {
		if (this.changed) {
			this.elem.style.display = "block";
						
			if (this.identity_id == -1 || this.contact_id == -1) {
				this.widget.elem.style.display = "none";
			} else {
				this.widget.elem.style.display = "block";
			}

			this.changed = false;
		}
	}
}
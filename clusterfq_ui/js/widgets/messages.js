var Messages = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.identity_id = -1;
	this.contact_id = -1;
	
	this.widget = new Widget("Messages");

	this.elem = this.widget.elem;
	this.elem.style.display = "none";
	
	this.changed = true;
	
	this.message_type = document.createElement("select");
	this.message_type.id = this.widget.name + "_type_select";
	this.message_type.style.display = "inline";
	
	this.message_type_option_text = document.createElement("option");	
	this.message_type_option_text.appendChild(document.createTextNode("Text"));
	this.message_type.appendChild(this.message_type_option_text);
	
	this.message_type_option_file = document.createElement("option");
	this.message_type_option_file.appendChild(document.createTextNode("File"));
	this.message_type.appendChild(this.message_type_option_file);
	
	this.widget.content.appendChild(this.message_type);
	
	this.on_message_send_initiated = function() {
		messagebox.message_add("message send initiated", 100, "no_class");
	}
	
	this.message_input = document.createElement("input");
	this.message_input.obj = this;
	this.message_input.id = this.widget.name + "_input";
	this.message_input.onkeydown = function() {
		if (event.key === 'Enter') {
			if (this.value.length > 0) {
				var input_text = this.value;
				this.value = "";
				
				var type = this.obj.message_type.options[this.obj.message_type.selectedIndex];
				if (type.innerHTML == "Text") {
					this.obj.db.query_post("message/send?identity_id=" + this.obj.identity_id + "&contact_id=" + this.obj.contact_id + "&type=text", input_text, this.on_message_send_initiated);
				} else if (type.innerHTML == "File") {
					this.obj.db.query_post("message/send?identity_id=" + this.obj.identity_id + "&contact_id=" + this.obj.contact_id + "&type=file", input_text, this.on_message_send_initiated);
				}
			}
		}
	}
	
	this.widget.content.appendChild(this.message_input);
	
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
						
			this.changed = false;
		}
	}
}
var Identities = function(db, change_dependencies) {
	this.db = db;
	this.change_dependencies = change_dependencies;
	
	this.identities_json = null;
	
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
	
	//Routine List
	this.routine_list = document.createElement("div");
	
	this.widget.content.appendChild(this.routine_list);

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
		db.query_post("identity_create?name=" + name, "{ }", identities.on_identity_create_response);
	}
	
	this.identities_loaded = false;
	
	this.on_identities_list_response = function() {
		identities.identities_json = JSON.parse(this.responseText);
	}
	
	this.update_identities_list = function() {
		db.query_get("identities_list", identities.on_identities_list_response);
		this.changed = true;
	}
	
	this.on_identities_load_response = function() {
		identities.update_identities_list();
	}

	this.update = function() {
		if (this.changed) {
			this.elem.style.display = "block";
			this.routine_list.innerHTML = "";
			
			this.update_add_view();
			
			if (!this.identities_loaded) {
				this.identities_loaded = true;
				this.db.query_post("identities_load", "{ }", identities.on_identities_load_response);
			}
			
			this.changed = false;
		}
	}
}
var DB = function(db_name) {
	this.db_name = db_name
	
	this.widget = new Widget("DB");

	this.elem = this.widget.elem;

	this.data = null;
	
	this.server_url = "http://localhost:8080/";
	
	this.query_get = function(url, callback) {
		var http = new XMLHttpRequest();
		http.addEventListener("load", callback);
		http.open("GET", this.server_url + url);
		http.send();
	}
	
	this.query_post = function(url, json, callback) {
		var http = new XMLHttpRequest();
		http.addEventListener("load", callback);
		http.open("POST", this.server_url + url);
		http.setRequestHeader("Content-Type", "application/json");
		http.send(json);
	}
		
	this.get_max_id = function(widget_name) {
		var max_id = -1;
		if (this.data[widget_name]) {
			for (var i = 0; i < this.data[widget_name].length; i++) {
				if (this.data[widget_name][i].Id > max_id) max_id = this.data[widget_name][i].Id;
			}
		}
		return max_id;
	}
	
	this.get_current_date = function() {
		var datenow = new Date();
		var month = parseInt(datenow.getMonth())+1;
		if (month < 10) {
			month = "0" + month;
		}
		var day = parseInt(datenow.getDate());
		if (day < 10) {
			day = "0" + day;
		}
		return datenow.getFullYear() + "-" + month + "-" + day;
	}

	this.get_current_datetime_as_obj = function() {
		var datenow = new Date();
		var year = parseInt(datenow.getFullYear());
		var month = parseInt(datenow.getMonth())+1;
		var day = parseInt(datenow.getDate());
		var hour = parseInt(datenow.getHours());
		var min = parseInt(datenow.getMinutes());
		var sec = parseInt(datenow.getSeconds());
		return {"Year": year, "Month": month, "Day": day, "Hour": hour, "Minute": min, "Second": sec};
	}

	this.update = function() {
		
	}
}
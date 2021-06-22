var MessageBox = function(messagebox_name) {
	this.elem = document.createElement('div');
	this.elem.id = "mb_" + messagebox_name;

	this.message_add = function(content_elem, frames, class_name, id, removable) {
		var msg_div = document.getElementById(this.elem.id + "_" + id);
		if (msg_div == undefined) {
			msg_div = document.createElement('div');
			this.elem.appendChild(msg_div);
		}
		msg_div.id = this.elem.id + "_" + id;
		msg_div.removable = removable;
		msg_div.innerHTML = "";
		msg_div.appendChild(content_elem);
		msg_div.frames = frames;
		msg_div.className = class_name;
		return msg_div;
	}

	this.message_remove = function(msg_div) {
		this.elem.removeChild(msg_div);
	}

	this.update = function() {
		var children = this.elem.childNodes;
		children.forEach(function(item) {
			if (item.frames > 0) {
				item.frames--;
			} else if (item.frames == 0){
				if (item.removable) {
					item.parentNode.removeChild(item);
				}
			}
		});
	}
}
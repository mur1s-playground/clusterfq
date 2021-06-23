var MessageBox = function(messagebox_name) {
	this.elem = document.createElement('div');
	this.elem.id = "mb_" + messagebox_name;

	this.msgs = {};

	this.message_add = function(content_elem, frames, class_name, id, removable) {
		var msg_div = null;
		if (this.msgs.hasOwnProperty(this.elem.id + "_" + id)) {
			msg_div = this.msgs[this.elem.id + "_" + id];
		}
		if (msg_div == null) {
			msg_div = document.createElement('div');
			this.msgs[this.elem.id + "_" + id] = msg_div;
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
		for (var c = 0; c < children.length; c++) {
			var item = children[c];
			if (item.frames > 0) {
				item.frames--;
			} else if (item.frames == 0){
				if (item.removable) {
					delete this.msgs[item.id];
					item.parentNode.removeChild(item);
				}
			}
		};
	}
}
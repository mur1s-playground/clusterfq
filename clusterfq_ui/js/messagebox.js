var MessageBox = function(messagebox_name) {
	this.elem = document.createElement('div');
	this.elem.id = "mb_" + messagebox_name;

	this.message_add = function(msg, frames, class_name) {
		var msg_div = document.createElement('div');
		msg_div.innerHTML = msg;
		msg_div.frames = frames;
		msg_div.className = class_name;
		this.elem.appendChild(msg_div);
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
				item.parentNode.removeChild(item);
			}
		});
	}
}
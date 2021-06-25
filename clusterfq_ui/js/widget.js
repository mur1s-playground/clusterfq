var Widget = function(name) {
	this.name = name;
	
	this.elem = document.createElement("div");
	this.elem.id = "wg_" + name;
	this.elem.className = "widget";
	
	this.arrow_down_img = document.createElement("img");
	this.arrow_down_img.src = "img/arrow_down.svg";
	this.arrow_down_img.className = "arrow_down";
	
	//-------------//
	//Widget Header//
	//-------------//
	this.header = document.createElement("div");
	this.header.id = "wg_" + name + "_header";
	this.header.className = "widget_header";
	
	this.header_content = document.createElement("div");
	
	this.header_content_tgl = document.createElement("a");
	this.header_content_tgl.obj = this;
	this.header_content_tgl.appendChild(this.arrow_down_img);
	this.header_content_tgl.onclick = function() {
		this.content = document.getElementById("wg_" + name + "_content");
		if (this.content.style.display == "none") {
			this.obj.arrow_down_img.className = "arrow_down";
			this.content.style.display = "block";
		} else {
			this.obj.arrow_down_img.className = "arrow_down right";
			this.content.style.display = "none";
		}
	}
	this.header_content.appendChild(this.header_content_tgl);
	
	this.header_headline = document.createElement("span");
	this.header_headline.innerHTML = name;
	this.header_content.appendChild(this.header_headline);

	this.header_var = document.createElement("div");
	this.header_var.id = this.name + "_header_var";
	this.header_var.className = "widget_header_var";
	this.header_content.appendChild(this.header_var);
	
	this.header.appendChild(this.header_content);
	this.elem.appendChild(this.header);
	
	//--------------//
	//Widget Content//
	//--------------//
	this.content = document.createElement("div");
	this.content.id = "wg_" + name + "_content";

	this.elem.appendChild(this.content);
}
var App = function(base_element) {
	this.base_element = base_element;

	this.components = [];

	this.component_add = function(parent, component) {
		this.components.push(component);
		if (component.elem != null) {
			parent.appendChild(component.elem);
		}
	}

	this.update = function() {
		this.components.forEach(function(item) {
			item.update();
		});
	}
}
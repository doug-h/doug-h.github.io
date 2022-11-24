var modal = document.getElementById('modal');
modal.style.display = "none";

var buttons = document.getElementsByClassName("demo-button");
var sliders = document.getElementsByClassName("variable-input");
var canvas = document.getElementById("canvas");

for (let i = 0; i < buttons.length; i++) {
    buttons[i].onclick = function() {
        modal.style.display = "inline";
        modal.current_demo = i+1;
        canvas.style.width  = buttons[i].dataset.width;
        canvas.style.height = buttons[i].dataset.height;

        for (let j = 0; j < sliders.length; j++) {
            if(sliders[j].dataset.demo == buttons[i].dataset.demo) {
                sliders[j].style.display = "inline";
            }
            else {
                sliders[j].style.display = "none";
            }
        }
    }
}

var close = document.getElementById("close");
close.onclick = function() {
    modal.style.display = "none";
    modal.current_demo = 0;
}

var slider_N_PART= document.getElementById("N_PART");
var label_N_PART = document.getElementById("label_N_PART");
slider_N_PART.value = 14;
label_N_PART.innerHTML = 1 << slider_N_PART.value;
slider_N_PART.oninput = function() {
    label_N_PART.innerHTML = 1 << this.value;
} 

var slider_P_SIZE_MIN= document.getElementById("P_SIZE_MIN");
var label_P_SIZE_MIN = document.getElementById("label_P_SIZE_MIN");
slider_P_SIZE_MIN.value = 2;
label_P_SIZE_MIN.innerHTML = slider_P_SIZE_MIN.value;
slider_P_SIZE_MIN.oninput = function() {
    label_P_SIZE_MIN.innerHTML = this.value;
}

var slider_P_SIZE_MAX= document.getElementById("P_SIZE_MAX");
var label_P_SIZE_MAX = document.getElementById("label_P_SIZE_MAX");
slider_P_SIZE_MAX.value = 2;
label_P_SIZE_MAX.innerHTML = slider_P_SIZE_MAX.value;
slider_P_SIZE_MAX.oninput = function() {
    label_P_SIZE_MAX.innerHTML = this.value;
}

var slider_CLOTH_GRID= document.getElementById("CLOTH_GRID");
var label_CLOTH_GRID = document.getElementById("label_CLOTH_GRID");
slider_CLOTH_GRID.value = 6;
label_CLOTH_GRID.innerHTML = 1 << slider_CLOTH_GRID.value;
slider_CLOTH_GRID.oninput = function() {
    label_CLOTH_GRID.innerHTML = 1 << this.value;
} 

//var slider_RUN_SPEED = document.getElementById("RUN_SPEED");
//var label_RUN_SPEED = document.getElementById("label_RUN_SPEED");
//slider_RUN_SPEED.value = 0;
//label_RUN_SPEED.innerHTML = "1&frasl;" + (1 << slider_RUN_SPEED.value);
//slider_RUN_SPEED.oninput = function() {
//  label_RUN_SPEED.innerHTML = "1&frasl;" + (1 << slider_RUN_SPEED.value);
//} 

var restart_button = document.getElementById("restart");
restart_button.onclick = function() {
  // Defined in C++ code
  _restart_demo();
}

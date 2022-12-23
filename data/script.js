document.addEventListener("DOMContentLoaded", function() {
	let greenhouseData = {}
	let ledData = {}
	let waterPumpData = {}
	let greenhouseDataValid = []
	let ledDataValid = []
	let waterPumpDataValid = []
	let greenhouseResetMessage = "Ustawienia szklarni zostały zresetowane!"
	let ledResetMessage = "Ustawienia lamp zostały zresetowane!"
	let waterPumpResetMessage = "Ustawienia pompy wody zostały zresetowane!"
	let time = new Date();
	let validationChecker = data => data.every(valid => valid === true)                  

	// ------------------- WEB SOCKET -------------------

	let webSocket;
	initWebSocket();

	function initWebSocket() {
		console.log('Nazwiązywanie połączenia z ESP32...');
		webSocket = new WebSocket(`ws://${window.location.hostname}/ws`);
		webSocket.onopen = onOpenWebSocket;
		webSocket.onclose = onCloseWebSocket;
		webSocket.onmessage = onMessageWebSocket;
	}

	function onOpenWebSocket(event) {
		console.log('Połączenie poprawnie nawiązane. Odbieranie danych...');
		webSocket.send("getValues");
		let time = '{"time" : ' + Date.now() + '}';
		console.log(time);
		webSocket.send(time);
	}

	function onCloseWebSocket(event) {
		console.log('Wystąpił problem podczas nawiązywania połączenia...');
		setTimeout(initWebSocket, 2000);
	}

	function onMessageWebSocket(event) {
		console.log(event.data);
		var myObj = JSON.parse(event.data);
		var keys = Object.keys(myObj);
		for (var i = 0; i < keys.length; i++){
			document.getElementById(keys[i]).innerHTML = myObj[keys[i]];
		}	
	}

	// ------------------- GLOBAL -------------------

	function toogleCheckbox(formElement, toogle, inputData) {
		document.querySelectorAll(formElement).forEach(function(element, index) {
			let checkbox = element.querySelector(toogle)
			let input = element.querySelector(inputData)
			input.disabled = true
			checkbox.disabled = false
			checkbox.addEventListener("click", function() {
				input.value = null
				input.disabled = !this.checked
			})
		})
	}

	function resetSettings(data, formElement, toogle, inputData, alertMessage, displaySelector, section){
		data = {}
		document.querySelectorAll(formElement).forEach(function(element, index) {
			let checkbox = element.querySelector(toogle)
			let input = element.querySelector(inputData)
			let displayValue = element.querySelector(displaySelector)
			checkbox.checked = false
			input.value = null
			input.disabled = true
			if(input.type == "range") {
				displayValue.innerHTML = ""
			}
		})
		data["stop"] = true
		console.log(data)
		alert(alertMessage)
	}

	function displayValues(formElement, toogle, inputData, displaySelector, section) {
		document.querySelectorAll(formElement).forEach(function(element,index) {
			let checkbox = element.querySelector(toogle)
			let input = element.querySelector(inputData)
			let displayValue = element.querySelector(displaySelector)
			document.getElementById(section).addEventListener("change", function(){
				if(input.type == "range") {
					displayValue.getAttribute("data-unit") == "degree" ? 
					checkbox.checked ? displayValue.innerHTML = input.value + "&#8451" : displayValue.innerHTML = "" :
					checkbox.checked ? displayValue.innerHTML = input.value + "%" : displayValue.innerHTML = ""
				}
			})
		})
	}

	// ------------------- GREENHOUSE -------------------

	toogleCheckbox('.greenhouse-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]')
	
	displayValues('.greenhouse-form-element',
				  '[data-action="greenhouse-checkbox-toggle"]',
				  '[data-action="greenhouse-input"]',
				  '[data-action="display-value"]',
				  "greenhouse")

	document.querySelector('[data-action="get-greenhouse-data"]').addEventListener("click", function() {
		greenhouseData = {}
		document.querySelectorAll('.greenhouse-form-element').forEach(function(element,index) {
			let checkbox = element.querySelector('[data-action="greenhouse-checkbox-toggle"]')
			let input = element.querySelector('[data-action="greenhouse-input"]')
			let displayValue = element.querySelector('[data-action="display-value"]')
			if(checkbox.checked && input.value !='') {
				greenhouseData[input.name] = input.value
				greenhouseDataValid[index] = true
			}
			if(checkbox.checked && input.value == '' && input.name == "greenhouse_time_off") {      
				greenhouseData = {}
				alert("Proszę wpisać godzinę, o której ma wyłączyć się regulacja!")
			} 
			if(!checkbox.checked) {
				delete greenhouseDataValid[index]
			}
		})
		if(Object.keys(greenhouseData).length > 0 && validationChecker(greenhouseDataValid)) {
			greenhouseData["current_time"] = time.getHours() + ":" + time.getMinutes() 
			console.log(greenhouseData)
			console.log(greenhouseDataValid)
		}
	})
	document.querySelector('[data-action="reset-greenhouse-data"]').addEventListener("click", function() {
		resetSettings(greenhouseData,
					  '.greenhouse-form-element',
					  '[data-action="greenhouse-checkbox-toggle"]',
					  '[data-action="greenhouse-input"]',
					  greenhouseResetMessage,
					  '[data-action="display-value"]',
					  "greenhouse")      

	})

	// ------------------- LED -------------------

	toogleCheckbox('.led-1-form-element', '[data-action="led-checkbox-toggle"]', '[data-action="led-input"]')

	document.querySelector('[data-action="get-led-data"]').addEventListener("click", function() {
		ledData = {}
		document.querySelectorAll('.led-1-form-element').forEach(function(element,index) {
			let checkbox = element.querySelector('[data-action="led-checkbox-toggle"]')
			let input = element.querySelector('[data-action="led-input"]')
			if(checkbox.checked && input.value !='') {
				ledData[input.name] = input.value
				ledDataValid[index] = true
			}
			if(checkbox.checked && input.value == '') {      
				ledData = {}
				ledDataValid[index] = false
				switch (input.name) {
					case "led_1": 
						alert("Proszę wpisać czas włączenia pierwszej lampy!")
						break
					case "led_1_time_off": 
						alert("Proszę wpisać czas wyłączenia pierwszej lampy!")
						break
				}
			} 
			if(!checkbox.checked) {
				delete ledDataValid[index]
			}
		})
		document.querySelectorAll('.led-2-form-element').forEach(function(element,index) {
			let checkbox = element.querySelector('[data-action="led-2-checkbox"]')       
			checkbox.checked ? ledData[checkbox.name] = true : ledData[checkbox.name] = false
		})
		if(Object.keys(ledData).length > 0 && validationChecker(ledDataValid)) {
			ledData["current_time"] = time.getHours() + ":" + time.getMinutes() 
			console.log(ledData)
			console.log(ledDataValid)
		}
	})

	document.querySelector('[data-action="reset-led-data"]').addEventListener("click", function() {
		resetSettings(greenhouseData,
					  '.led-1-form-element',
					  '[data-action="led-checkbox-toggle"]',
					  '[data-action="led-input"]',
					  ledResetMessage,
					  '[data-action="display-value"]',
					  "led")
		document.querySelectorAll('.led-2-form-element').forEach(function(element,index) {
			let checkbox = element.querySelector('[data-action="led-2-checkbox"]')
			checkbox.checked = false
		})  
	})

	// ------------------- WATER PUMP -------------------

	toogleCheckbox('.water-pump-form-element', '[data-action="water-pump-checkbox-toggle"]', '[data-action="water-pump-input"]')

	displayValues('.water-pump-form-element',
				  '[data-action="water-pump-checkbox-toggle"]',
				  '[data-action="water-pump-input"]',
				  '[data-action="display-value"]',
				  "water-pump")

	document.querySelector('[data-action="get-water-pump-data"]').addEventListener("click", function() {
		waterPumpData = {}
		document.querySelectorAll('.water-pump-form-element').forEach(function(element,index) {
			let checkbox = element.querySelector('[data-action="water-pump-checkbox-toggle"]')
			let input = element.querySelector('[data-action="water-pump-input"]')
			if(checkbox.checked && input.value !='') {
				waterPumpData[input.name] = input.value
				waterPumpDataValid[index] = true
			}
			if(!checkbox.checked) {
				delete waterPumpDataValid[index]
			}
		})
		if(Object.keys(waterPumpData).length > 0 && validationChecker(waterPumpDataValid)) {
			waterPumpData["current_time"] = time.getHours() + ":" + time.getMinutes() 
			console.log(waterPumpData)
			console.log(waterPumpDataValid)
		}
	})

	document.querySelector('[data-action="reset-water-pump-data"]').addEventListener("click", function() {
		resetSettings(waterPumpData,
					  '.water-pump-form-element',
					  '[data-action="water-pump-checkbox-toggle"]',
					  '[data-action="water-pump-input"]',
					  waterPumpResetMessage,
					  '[data-action="display-value"]',
					  "water-pump")
	})
})
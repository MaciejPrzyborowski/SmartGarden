document.addEventListener("DOMContentLoaded", function() {
	let greenhouseData = {}
	let ledData = {}
	let waterPumpData = {}
	let greenhouseResetMessage = "Ustawienia szklarni zostały zresetowane!"
	let ledDataError = "Nie podano godziny w konfiguracji lampy 1!"
	let ledResetMessage = "Ustawienia lamp zostały zresetowane!"
	let waterPumpResetMessage = "Ustawienia pompy wody zostały zresetowane!"
	let ErrortMessage = "Nie wybrano żadnych opcji!"
	let validationChecker = data => data.every(valid => valid === true)

	// ---------------- WEB SOCKET --------------------

	let webSocket;
	
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
	}

	function onCloseWebSocket(event) {
		console.log('Wystąpił problem podczas nawiązywania połączenia...');
		setTimeout(initWebSocket, 2000);
	}

	function onMessageWebSocket(event) {
		console.log(event.data)
		var obj = JSON.parse(event.data)
		var key = Object.keys(obj)
		for (var i = 0; i < key.length; i++){
			if(key[i] == "gh_auto_mode") {
				if(obj[key[i]] == true) {
					greenhouse_manual_mode = false
					document.querySelector("#gh_auto_mode").checked = true
					document.querySelector("#gh_manual_mode").checked = false
				} else if(obj[key[i]] == false) {
					greenhouse_manual_mode = true
					document.querySelector("#gh_auto_mode").checked = false
					document.querySelector("#gh_manual_mode").checked = false
				}
			}
			else if(key[i] == "gh_manual_state") {
				if(obj[key[i]] == false && greenhouse_manual_mode == true) {
					document.querySelector("#gh_manual_on").checked = false
					document.querySelector("#gh_manual_off").checked = true
					document.querySelector("#gh_manual_mode").checked = true
				} else if (obj[key[i]] == false && greenhouse_manual_mode == false) {
					document.querySelector("#gh_manual_on").checked = false
					document.querySelector("#gh_manual_off").checked = false
					document.querySelector("#gh_manual_mode").checked = false
				} else if (obj[key[i]] == true && greenhouse_manual_mode == true) {
					document.querySelector("#gh_manual_on").checked = true
					document.querySelector("#gh_manual_off").checked = false
					document.querySelector("#gh_manual_mode").checked = true
				}
			}
			else if(key[i] == "gh_target_temp") {
				if(obj[key[i]] == 0) {
					document.querySelector('#gh_target_temp_is_set').checked = false
				} else {
					document.querySelector('#gh_target_temp_is_set').checked = true
					document.querySelector('[name="gh_target_temp"]').value = obj[key[i]]
					document.querySelector('[name="gh_target_temp"]').disabled = false
					displayValues('.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]', '[data-action="greenhouse-display-value"]')
				}
			}
			else if(key[i] == "gh_temp_off") {
				if(obj[key[i]] == 0) {
					document.querySelector('#gh_temp_off_is_set').checked = false
					document.querySelector('[name="gh_temp_off"]').disabled = true
				} else {
					document.querySelector('#gh_temp_off_is_set').checked = true
					document.querySelector('[name="gh_temp_off"]').value = obj[key[i]]
					document.querySelector('[name="gh_temp_off"]').disabled = false
					displayValues('.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]', '[data-action="greenhouse-display-value"]')
				}
			}
			else {
				document.getElementById(key[i]).innerHTML = obj[key[i]]
			}
		}
		toogleVisibility(document.querySelector('[name="greenhouse_mode"]:checked').value,
		document.querySelector('.gh-auto-mode'),
		document.querySelector('.gh-manual-mode'),
		null, null, null, null)
	}	
	

// input radio           gh_auto_mode: false 
// input radio           gh_manual_mode: false
// input radio           gh_manual_state: false
// input range           gh_target_temp: 0
// input range           gh_temp_off: 0
// input checkbox        gh_temp_off_is_set: false

	initWebSocket();

	// ------------------ GLOBAL ----------------------

	function toogleCheckbox(formElement, toogle, inputData) {
		document.querySelectorAll(formElement).forEach(function(element, index) {
			let checkbox = element.querySelector(toogle)
			let input = element.querySelector(inputData)
			input.disabled = true
			// checkbox.disabled = false //to usunac
			checkbox.addEventListener("click", function() {
				input.value = null
				input.disabled = !this.checked
			})
			checkbox.checked ? input.disabled = false : input.disabled = true
			checkbox.checked ? null : input.value = null
		})
	}

	function displayValues(formElement, toogle, inputData, displaySelector) {
		document.querySelectorAll(formElement).forEach(function(element,index) {
			let checkbox = element.querySelector(toogle)
			let input = element.querySelector(inputData)
			let displayValue = element.querySelector(displaySelector)
			if(input.type == "range") {
				switch(displayValue.getAttribute("data-unit")) {
					case "degrees":
						checkbox.checked ? displayValue.innerHTML = input.value + "&#8451" : displayValue.innerHTML = ""
						break
					case "percents":
						checkbox.checked ? displayValue.innerHTML = input.value + "%" : displayValue.innerHTML = ""
						break
					case "minutes":
						checkbox.checked ? displayValue.innerHTML = input.value + " minut" : displayValue.innerHTML = ""
						break
				}
			}
		})
	}

	function timeSplitter(time) {
		let data = time.split(':')
		return data
	}

	function toogleVisibility(radioValue, autoElement ,manualElement, manualRadio, mainCheckbox, autoFormElement, checkboxToogleAction) {
		switch(radioValue) {
			case "auto":
				// autoElement.style.border = "1px solid green"
				// manualElement.style.border = "1px solid red"
				autoElement.style.display = "inline-block"
				manualElement.style.display = "none"
				if(manualRadio != null) {
					document.querySelectorAll(manualRadio).forEach(function(element, index) {
						element.checked = false
					})
				}
				mainCheckbox != null? mainCheckbox.checked = true : null
				if(mainCheckbox != null) {
					mainCheckbox.checked = true
				} else if(autoFormElement != null) {
					let autoOn = document.querySelector(autoFormElement)
					autoOn.checked = true
				}

				break
			case "manual":
				autoElement.style.display = "none"
				manualElement.style.display = "inline-block"

				document.querySelectorAll(autoFormElement).forEach(function(element, index) {
					if(checkboxToogleAction != null) {
						let checkbox = element.querySelector(checkboxToogleAction)
						checkbox.checked = false
					} else {
						element.checked = false
					}
				})
				if(manualRadio != null) {
					let manualOn = document.querySelector(manualRadio)
					manualOn.checked = true
				}

				break
			default: 
			autoElement.style.display = "none"
			manualElement.style.display = "none"

				document.querySelectorAll(autoFormElement).forEach(function(element, index) {
					if(checkboxToogleAction != null) {
						let checkbox = element.querySelector(checkboxToogleAction)
						checkbox.checked = false
					}
					else {
						element.checked = false
					}
				})
				document.querySelectorAll(manualRadio).forEach(function(element, index) {
					element.checked = false
				})

				break
		}
	}

	document.addEventListener("change", function(){
		displayValues('.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]', '[data-action="greenhouse-display-value"]')
		displayValues('.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]', '[data-action="water-pump-input"]', '[data-action="water-pump-display-value"]')
	})

	// ------------------ GREENHOUSE ----------------------
	document.querySelectorAll('[name="greenhouse_mode"]').forEach(function(element, index){
		element.addEventListener("click", function(){
			let targetTempCheckbox = document.querySelector('#gh_target_temp_is_set')
			let radioValue = document.querySelector('[name="greenhouse_mode"]:checked').value
			let autoElement = document.querySelector('.gh-auto-mode')
			let manualElement = document.querySelector('.gh-manual-mode')
			toogleVisibility(radioValue, autoElement, manualElement, '[name="gh_manual"]', targetTempCheckbox, '.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]')
			toogleCheckbox('.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]')
		})
	})
	document.querySelector('[data-action="get-greenhouse-data"]').addEventListener("click", function() {
        greenhouseData = {"cfg": "greenhouse"}
        let validation = false
        document.querySelectorAll('[name="greenhouse_mode"]').forEach(function(element, index){
            element.checked ? validation = true : null
        })

        if(validation) {            
            document.querySelectorAll('[data-element="greenhouse-form"]').forEach(function(element, index){
                if(element.name == "gh_manual") {
                    let value 
                    if(document.querySelector('#gh_manual_mode').checked) {
                        value= document.querySelector('[name="gh_manual"]:checked').value
                    } else {
                        value = "off"
                    }
                    greenhouseData["gh_manual_state"] = value == "on"? true : false
                }
                else {
                    greenhouseData[element.id] = element.checked
                }
            })
            document.querySelectorAll('[data-action="greenhouse-input"]').forEach(function(element, index){
                element.disabled? greenhouseData[element.name] = 0 : greenhouseData[element.name] = parseInt(element.value)
            })
            // console.log(greenhouseData)
            let jsonObj = JSON.stringify(greenhouseData)
            webSocket.send(jsonObj)
        } else {
            alert(ErrortMessage)
        }
    })
	document.querySelector('[data-action="reset-greenhouse-data"]').addEventListener("click", function() {
		greenhouseData = {"cfg": "greenhouse"}
		let autoElement = document.querySelector('.gh-auto-mode')
		let manualElement = document.querySelector('.gh-manual-mode')

		document.querySelectorAll('[data-element="greenhouse-form"]').forEach(function(element, index){
			element.checked = false
		})

		document.querySelectorAll('[data-action="greenhouse-display-value"]').forEach(function(element, index){
			element.innerHTML = ""
		})

		document.querySelectorAll('[data-element="greenhouse-form"]').forEach(function(element, index){
			if(element.name == "gh_manual") {
				greenhouseData["gh_manual_state"] = false
			}
			else {
				greenhouseData[element.id] = false
			}
		})
		document.querySelectorAll('[data-action="greenhouse-input"]').forEach(function(element, index){
			greenhouseData[element.name] = 0
		})
        toogleVisibility("manual", autoElement, manualElement, '[name="gh_manual"]', null, '.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]')
        toogleCheckbox('.greenhouse-auto-form-element', '[data-action="greenhouse-checkbox-toggle"]', '[data-action="greenhouse-input"]')
        
        document.querySelector('#gh_manual_off').checked = true
        document.querySelector('#gh_manual_mode').checked = true

		alert(greenhouseResetMessage)

		//console.log(greenhouseData)
		let jsonObj = JSON.stringify(greenhouseData)
		webSocket.send(jsonObj)

	})

	// --------------------- LEDS -------------------------
	document.querySelectorAll('[name="led1_mode"]').forEach(function(element, index){
		element.addEventListener("click", function(){
			let led1TimeOnCheckbox = document.querySelector('#l1_time_on_is_set')
			let radioValue = document.querySelector('[name="led1_mode"]:checked').value
			let autoElement = document.querySelector('.led-1-auto-mode')
			let manualElement = document.querySelector('.led-1-manual-mode')
			toogleVisibility(radioValue, autoElement, manualElement, '[name="l1_manual"]', led1TimeOnCheckbox, '.led-1-auto-form-element', '[data-action="led-1-checkbox-toggle"]')
			toogleCheckbox('.led-1-auto-form-element', '[data-action="led-1-checkbox-toggle"]', '[data-action="led-1-input"]')
		})
	})
	document.querySelectorAll('[name="led2_mode"]').forEach(function(element, index){
		element.addEventListener("click", function(){
			let radioValue = document.querySelector('[name="led2_mode"]:checked').value
			let autoElement = document.querySelector('.led-2-auto-mode')
			let manualElement = document.querySelector('.led-2-manual-mode')
			toogleVisibility(radioValue, autoElement, manualElement, '[name="l2_manual"]', null, '[name="l2_auto"]', null)
		})
	})
	document.querySelector('[data-action="get-led-data"]').addEventListener("click", function() {
		ledData = {"cfg": "led"}
		let formValidation = false
		let dataValidation = new Array()

		document.querySelectorAll('.leds_mode').forEach(function(element, index){
			element.checked ? formValidation = true : null
		})

		document.querySelectorAll('.led-1-auto-form-element').forEach(function(element, index){
			let checkbox = element.querySelector('[data-action="led-1-checkbox-toggle"]')
			let input = element.querySelector('[data-action="led-1-input"]')
			if(checkbox.checked && input.value != "") {
				dataValidation[index] = true 
			} else if(checkbox.checked && input.value == "") {
				dataValidation[index] = false
			} else if(!checkbox.checked){
				dataValidation[index] = true
			} else {
				dataValidation[index] = false
			}
		})

		if(formValidation) {
			if(validationChecker(dataValidation)) {
				document.querySelectorAll('[data-element="led-form"]').forEach(function(element, index){

					if(element.name == "l1_manual") {
						let value 
						if(document.querySelector('#l1_manual_mode').checked) {
							value= document.querySelector('[name="l1_manual"]:checked').value
						} else {
							value = "off"
						}
						ledData["l1_manual_state"] = value == "on"? true : false
					} else if (element.name == "l2_manual") {
						let value 
						if(document.querySelector('#l2_manual_mode').checked) {
							value = document.querySelector('[name="l2_manual"]:checked').value
						} else {
							value = "off"
						}
						ledData["l2_manual_state"] = value == "on"? true : false
					} else if (element.name == "l2_auto") {
						let value
						if(document.querySelector('#l2_auto_mode').checked) {
							let x = document.querySelector('[name="l2_auto"]:checked').value
							value = x == "dusk" ? value = 1 : x == "motion" ? value = 2 : x == "dusk_motion"? value = 3 : null
							
						} else {
							value = 0
						}
						ledData["l2_detect_mode"] = value
					}
					else {
						ledData[element.id] = element.checked
					}
				})
				document.querySelectorAll('[data-action="led-1-input"]').forEach(function(element, index){
					if(!element.disabled) {
						ledData[element.name+"_hour"] = parseInt(timeSplitter(element.value)[0])
						ledData[element.name+"_min"] = parseInt(timeSplitter(element.value)[1])
					} else {
						ledData[element.name+"_hour"] = 0
						ledData[element.name+"_min"] = 0
					}
				})
				
				let jsonObj = JSON.stringify(ledData)
				webSocket.send(jsonObj)
				//console.log(jsonObj)
			} else {
				alert(ledDataError)
			}		
		} else {
			alert(ErrortMessage)
		}
	})
	document.querySelector('[data-action="reset-led-data"]').addEventListener("click", function() {
		ledData = {"cfg": "led"}
		let led1AutoElement = document.querySelector('.led-1-auto-mode')
		let led1ManualElement = document.querySelector('.led-1-manual-mode')
		let led2AutoElement = document.querySelector('.led-2-auto-mode')
		let led2ManualElement = document.querySelector('.led-2-manual-mode')

		document.querySelectorAll('[data-element="led-form"]').forEach(function(element, index){
			element.checked = false
		})

		toogleVisibility(null, led1AutoElement, led1ManualElement, '[name="l1_manual"]', null, '.led-1-auto-form-element', '[data-action="led-1-checkbox-toggle"]')
		toogleVisibility(null, led2AutoElement, led2ManualElement, '[name="l2_manual"]', null, '.led-2-auto-form-element', null)
		toogleCheckbox('.led-1-auto-form-element', '[data-action="led-1-checkbox-toggle"]', '[data-action="led-1-input"]')

		document.querySelectorAll('[data-element="led-form"]').forEach(function(element, index){
			if(element.name == "l1_manual") {
				ledData["l1_manual_state"] = false
			} else if (element.name == "l2_manual") {
				ledData["l2_manual_state"] = false
			} else if (element.name == "l2_auto") {
				ledData["l2_auto_work_mode"] = 0
			}
			else {
				ledData[element.id] = false
			}

		})
		document.querySelectorAll('[data-action="led-1-input"]').forEach(function(element, index){
			ledData[element.name+"_hour"] = 0
			ledData[element.name+"_min"] = 0
		})

		alert(ledResetMessage)

		//console.log(ledData)
		let jsonObj = JSON.stringify(ledData)
		webSocket.send(jsonObj)
	})

	// ------------------ WATER-PUMP ----------------------
	document.querySelectorAll('[name="water_pump_mode"]').forEach(function(element, index){
		element.addEventListener("click", function(){
			let waterPumpHumidityBelowCheckbox = document.querySelector('#wp_humidity_below_is_set')
			let radioValue = document.querySelector('[name="water_pump_mode"]:checked').value
			let autoElement = document.querySelector('.water-pump-auto-mode')
			let manualElement = document.querySelector('.water-pump-manual-mode')
			toogleVisibility(radioValue, autoElement, manualElement, '[name="wp_manual"]', waterPumpHumidityBelowCheckbox, '.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]')
			toogleCheckbox('.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]', '[data-action="water-pump-input"]')
		})
	})
	document.querySelector('[data-action="get-water-pump-data"]').addEventListener("click", function() {
		waterPumpData = {"cfg": "water_pump"}
		let validation = false
		document.querySelectorAll('[name="water_pump_mode"]').forEach(function(element, index){
			element.checked ? validation = true : null
		})

		if(validation) {			
			document.querySelectorAll('[data-element="water-pump-form"]').forEach(function(element, index){
				if(element.name == "wp_manual") {
					if(document.querySelector('#wp_manual_mode').checked) {
						value= document.querySelector('[name="wp_manual"]:checked').value
					} else {
						value = "off"
					}
					waterPumpData["wp_manual_state"] = value == "on"? true : false
				}
				else {
					waterPumpData[element.id] = element.checked
				}
			})

			document.querySelectorAll('[data-action="water-pump-input"]').forEach(function(element, index){
				element.disabled? waterPumpData[element.name] = 0 : waterPumpData[element.name] = element.value
			})
			//console.log(waterPumpData)
			let jsonObj = JSON.stringify(waterPumpData)
			webSocket.send(jsonObj)
		} else {
			alert(ErrortMessage)
		}
	})
	document.querySelector('[data-action="reset-water-pump-data"]').addEventListener("click", function() {
		waterPumpData = {"cfg": "water_pump"}
		let autoElement = document.querySelector('.water-pump-auto-mode')
		let manualElement = document.querySelector('.water-pump-manual-mode')

		document.querySelectorAll('[data-element="water-pump-form"]').forEach(function(element, index){
			element.checked = false
		})
		toogleCheckbox('.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]', '[data-action="water-pump-input"]')
		document.querySelectorAll('[data-action="water-pump-display-value"]').forEach(function(element, index){
			element.innerHTML = ""
		})

		toogleVisibility(null, autoElement, manualElement, '[name="wp_manual"]', null, '.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]')
		toogleCheckbox('.water-pump-auto-form-element', '[data-action="water-pump-checkbox-toggle"]', '[data-action="water-pump-input"]')

		document.querySelectorAll('[data-element="water-pump-form"]').forEach(function(element, index){
			if(element.name == "wp_manual") {
				waterPumpData["wp_manual_state"] = false
			}
			else {
				waterPumpData[element.id] = false
			}
		})
		document.querySelectorAll('[data-action="water-pump-input"]').forEach(function(element, index){
			waterPumpData[element.name] = 0
		})

		alert(waterPumpResetMessage)

		//console.log(waterPumpData)
		let jsonObj = JSON.stringify(waterPumpData)
		webSocket.send(jsonObj)
	})
})
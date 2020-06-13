/**
*  Arduino Relay Child
*
*  Author: 
*    Henry Groover
*
*  Documentation:  [Does not exist, yet]
*
*  Changelog:
*    1.0 13-Jun-2020
*        Initial version
*
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
*  in compliance with the License. You may obtain a copy of the License at:
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
*  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
*  for the specific language governing permissions and limitations under the License.
*
*/

metadata {
definition (
    name: "Arduino Relay Child", 
    namespace: "hgroover", 
    author: "Henry Groover",
    importUrl: "https://raw.githubusercontent.com/hgroover/hubitat-arduino-relay/master/Hubitat/device-child.groovy") {
    
        capability "Actuator"
		capability "Initialize"
        capability "Polling"
        capability "Refresh"
		capability "Sensor"
		capability "Switch"
		capability "Momentary"
	}
    preferences {  
         // No preferences for this device
    }
}

def on(){
    // Turn the device on
    
    // sendEvent is handled by parent device
    def deviceType = device.deviceNetworkId.split("-")
    if(deviceType.length == 3){
        // Device Type is Relay

        if(deviceType[2] == "1"){
            // Relay 1
            parent.relay1On()
        } else if (deviceType[2] == "2") {
            // Relay 2
            parent.relay2On()
        } else if (deviceType[2] == "3") {
			// Relay 3
			parent.relay3On()
		} else if (deviceType[2] == "4") {
			// Relay 4
			parent.relay4On()
		}
    }   
}
def off(){
    // Turn the device off
    
    // sendEvent is handled by parent device
    def deviceType = device.deviceNetworkId.split("-")
if(deviceType.length == 3){
        // Device Type is Relay

        if(deviceType[2] == "1"){
            // Relay 1
            parent.relay1Off()
        } else if (deviceType[2] == "2") {
            // Relay 2
            parent.relay2Off()
        } else if (deviceType[2] == "3") {
			// Relay 3
			parent.relay3Off()
		} else if (deviceType[2] == "4") {
			// Relay 4
			parent.relay4Off()
		}
    }   
}

def push(){
	// 500ms push (garage door open/close button)
	def deviceType = device.deviceNetworkId.split("-")
	if (deviceType.length == 3){
		if  (deviceType[2] == "1"){
			parent.relay1Push()
		} else if (deviceType[2] == "2") {
			parent.relay2Push()
		} else if (deviceType[2] == "3") {
			parent.relay3Push()
		} else if (deviceType[2] == "4") {
			parent.relay4Push()
		}
	}
}

def initialize(){
    // Do nothing
    
}

def updated(){
    // Do nothing
    
}

def refresh(){
    // Request an info packet from the gateway
    
    parent.refresh()
}

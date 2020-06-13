/**
 *  Arduino relay device handler
 *
 *  Copyright 2020 Henry Groover.  All rights reserved.
 *
 *  2020-06-12: Initial version for Hubitat, cloned from WinkRelay device
 *      The final code in this thread was also invaluable: https://github.com/DaveGut/Hubitat-TP-Link-Integration/issues/6
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
    definition (name: "Arduino Relay", namespace: "hgroover", author: "Henry Groover", importURL: "https://raw.githubusercontent.com/hgroover/hubitat-arduino-relay/master/Hubitat/device.groovy") {
        capability "Switch"
		capability "Momentary"
		
        capability "Polling"
        capability "Refresh"
        capability "Configuration"
	    
        attribute "relay1", "enum", ["on", "off"]
        command "relay1On"
        command "relay1Off"
		command "relay1Push"

        attribute "relay2", "enum", ["on", "off"]
        command "relay2On"
        command "relay2Off"
		command "relay2Push"

        attribute "relay3", "enum", ["on", "off"]
        command "relay3On"
        command "relay3Off"
		command "relay3Push"

        attribute "relay4", "enum", ["on", "off"]
        command "relay4On"
        command "relay4Off"
		command "relay4Push"

		attribute "requestNumber", "number"
    } 
    preferences {
        
        input(name:"logDebug", type:"bool", title: "Log debug information?",
                  description: "Logs raw data for debugging. (Default: Off)", defaultValue: false,
                  required: true, displayDuringSetup: true)

        input(name:"logDescriptionText", type:"bool", title: "Log descriptionText?",
                  description: "Logs when things happen. (Default: On)", defaultValue: true,
                  required: true, displayDuringSetup: true)
    }
}

def installed(){
    def networkID = device.deviceNetworkId
    
	device.requestNumber = 101
    createChildDevices()
    refresh()
    //setupEventSubscription() - refresh includes this now
}
def updated(){
    sendEvent(name:"numberOfButtons", value:4)
    refresh()
    //setupEventSubscription() - refresh includes this now
}

def configure(){
    createChildDevices()
}

def createChildDevices(){

    def networkID = device.deviceNetworkId
	
    try{
       addChildDevice("hgroover", "Arduino Relay Child", "${networkID}-Relay-1", [label: "ArdRelay 1", isComponent: true])
    } catch(child1Error){ logDebug "Child 1 has already been created" }
    try{
        addChildDevice("hgroover", "Arduino Relay Child", "${networkID}-Relay-2", [label: "ArdRelay 2", isComponent: true])
    } catch(child2Error){ logDebug "Child 2 has already been created" }
    try{
        addChildDevice("hgroover", "Arduino Relay Child", "${networkID}-Relay-3", [label: "ArdRelay 3", isComponent: true])
    } catch(child2Error){ logDebug "Child 3 has already been created" }
    try{
        addChildDevice("hgroover", "Arduino Relay Child", "${networkID}-Relay-4", [label: "ArdRelay 4", isComponent: true])
    } catch(child2Error){ logDebug "Child 4 has already been created" }
}

// parse events into attributes
def parse(String description) {
    logDebug "Parsing '${description}'"
    def msg = parseLanMessage(description)
    //logDebug "JSON: ${msg.json}"
    def networkID = device.deviceNetworkId

    childRelay1 = getChildDevice("${networkID}-Relay-1")
    childRelay2 = getChildDevice("${networkID}-Relay-2")
	childRelay3 = getChildDevice("${networkID}-Relay-3")
	childRelay4 = getChildDevice("${networkID}-Relay-4")

    if(msg?.json?.Relay1){
        logDescriptionText "Relay 1: ${msg.json.Relay1}"
        childRelay1.sendEvent(name: "switch", value: msg.json.Relay1)        
    }
    if(msg?.json?.Relay2){
        logDescriptionText "Relay 2: ${msg.json.Relay2}"
        childRelay2.sendEvent(name: "switch", value: msg.json.Relay2)      
    }
    if(msg?.json?.Relay3){
        logDescriptionText "Relay 3: ${msg.json.Relay3}"
        childRelay3.sendEvent(name: "switch", value: msg.json.Relay3)      
    }
    if(msg?.json?.Relay4){
        logDescriptionText "Relay 4: ${msg.json.Relay4}"
        childRelay4.sendEvent(name: "switch", value: msg.json.Relay4)      
    }
  
	// hgroover - unclear what the parent device switch state is used for...
	
    //if both relays are on and the switch isn't currently on, let's raise that value
    if((childRelay1.currentValue("switch") == "on" || childRelay2.currentValue("switch") == "on") && device.currentValue("switch") != "on"){
        sendEvent(name: "switch", value: "on")
    }
    //and same in reverse
    if(childRelay1.currentValue("switch") == "off" && childRelay2.currentValue("switch") == "off" && device.currentValue("switch") != "off"){
        sendEvent(name: "switch", value: "off")
    }
    
    
}


def roundValue(x){
	Math.round(x * 10) / 10
}

def parseProximity(proxRaw){
	//split on spaces and grab the first value
    proxRaw.split(" ")[0] as Integer
}

//for now, we'll just have these turn on both relays
//in the future, we plan on providing the ability to disable either relay via the Android app
def on(){
    def action = []
    action << relay1On()
    action << relay2On()
    return action
}
def off(){
    def action = []
    action << relay1Off()
    action << relay2Off()
    return action
}
def push(){
}

//TODO: change actions to POST commands on the server and here
def relay1On(){
    def networkID = device.deviceNetworkId
    childRelay1 = getChildDevice("${networkID}-Relay-1")
    childRelay1.sendEvent(name: "switch", value: "on")
    //httpGET("/relay/top/on")
	tcpSend("a=1")
}
def relay1Off(){
    def networkID = device.deviceNetworkId
    childRelay1 = getChildDevice("${networkID}-Relay-1")
    childRelay1.sendEvent(name: "switch", value: "off")
    //httpGET("/relay/top/off")
	tcpSend("a=0")
}
def relay1Toggle(){
    def networkID = device.deviceNetworkId
    childRelay1 = getChildDevice("${networkID}-Relay-1")
    childRelay1.currentValue("switch") == "on" ? relay1Off() : relay1On()
}
def relay1Push(){
	def networkID = device.deviceNetworkId
	childRelay1 = getChildDevice("${networkID}-Relay-1")
	// No need to send event?
    childRelay1.sendEvent(name: "switch", value: "off")
	tcpSend("a=5")
}

def relay2On(){   
    def networkID = device.deviceNetworkId
    childRelay2 = getChildDevice("${networkID}-Relay-2")
    childRelay2.sendEvent(name: "switch", value: "on")
    //httpGET("/relay/bottom/on")
	tcpSend("b=1")
}
def relay2Off(){
    def networkID = device.deviceNetworkId
    childRelay2 = getChildDevice("${networkID}-Relay-2")
    childRelay2.sendEvent(name: "switch", value: "on")    
    //httpGET("/relay/bottom/off")
	tcpSend("b=0")
}
def relay2Toggle(){
    def networkID = device.deviceNetworkId
    childRelay2 = getChildDevice("${networkID}-Relay-2")
    childRelay2.currentValue("switch") == "on" ? relay2Off() : relay2On()
}
def relay2Push(){
	def networkID = device.deviceNetworkId
	childRelay2 = getChildDevice("${networkID}-Relay-2")
	// No need to send event?
	tcpSend("a=5")
}

def relay3On(){   
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-3")
    childRelay.sendEvent(name: "switch", value: "on")
    //httpGET("/relay/bottom/on")
	tcpSend("c=1")
}
def relay3Off(){
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-3")
    childRelay.sendEvent(name: "switch", value: "on")    
    //httpGET("/relay/bottom/off")
	tcpSend("c=0")
}
def relay3Toggle(){
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-3")
    childRelay.currentValue("switch") == "on" ? relay3Off() : relay3On()
}
def relay3Push(){
	def networkID = device.deviceNetworkId
	childRelay = getChildDevice("${networkID}-Relay-3")
	// No need to send event?
	tcpSend("c=5")
}

def relay4On(){   
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-4")
    childRelay.sendEvent(name: "switch", value: "on")
    //httpGET("/relay/bottom/on")
	tcpSend("d=1")
}
def relay4Off(){
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-4")
    childRelay.sendEvent(name: "switch", value: "on")    
    //httpGET("/relay/bottom/off")
	tcpSend("d=0")
}
def relay4Toggle(){
    def networkID = device.deviceNetworkId
    childRelay = getChildDevice("${networkID}-Relay-4")
    childRelay.currentValue("switch") == "on" ? relay4Off() : relay4On()
}
def relay4Push(){
	def networkID = device.deviceNetworkId
	childRelay = getChildDevice("${networkID}-Relay-4")
	// No need to send event?
	tcpSend("d=5")
}


//Individual commands for retrieving the status of the Wink Relay over HTTP
def retrieveRelay1(){ httpGET("/relay/top") }
def retrieveRelay2(){ httpGET("/relay/bottom")}

def setupEventSubscription(){
    logDebug "Subscribing to events from Arduino Relay"
    def result = new hubitat.device.HubAction(
            method: "POST",
            path: "/subscribe",
            headers: [
                    HOST: getHostAddress(),
                    CALLBACK: getCallBackAddress()
            ]
    )
    //logDebug "Request: ${result.requestId}"
    return result
}



def poll(){
    refresh()
}

def refresh(){
    //manually get the state of sensors and relays via TCP query
    def httpCalls = []
    httpCalls << retrieveRelay1()
    httpCalls << retrieveRelay2()
    httpCalls << setupEventSubscription()
    return httpCalls
}


def sync(ip, port) {
    def existingIp = getDataValue("ip")
    def existingPort = getDataValue("port")
    if (ip && ip != existingIp) {
        updateDataValue("ip", ip)
    }
    if (port && port != existingPort) {
        updateDataValue("port", port)
    }
    refresh()
}

def tcpSend(msg) {
    def hostAddrPort = getHostAddress()
	def requestNumber = 100
	if (getDataValue("requestNumber") != null){
		requestNumber = Integer.parseInt(getDataValue("requestNumber"),10)
	}
	else log.warn "undefined requestNumber"
	if (requestNumber>=999){
		requestNumber = 1
	}
	updateDataValue("requestNumber",(requestNumber+1).toString())
	def rnStr = "000" + requestNumber.toString()
	rnStr = rnStr.substring(rnStr.length() - 3)
    def fullMsg = "rn=${rnStr};${msg}"
	logDebug "Sending ${fullMsg} to ${hostAddrPort}"
	def hostParts = hostAddrPort.split(":")
	if (hostParts.length != 2){
		return
	}
	def hostIp = hostParts[0]
	def hostPort = Integer.parseInt(hostParts[1], 10)
	interfaces.rawSocket.connect(hostIp, hostPort)
	interfaces.rawSocket.sendMessage(fullMsg)
	runIn(3, closeSocket)
}
def closeSocket() {
	interfaces.rawSocket.close()
}
def socketStatus(response) {
	log.warn "TCP Socket is closed"
}

def httpGET(path) {
	def hostUri = hostAddress
    logDebug "Sending command ${path} to ${hostUri}"
    def result = new hubitat.device.HubAction(
            method: "GET",
            path: path,
            headers: [
                    HOST: hostUri
            ]
    )
    //logDebug "Request: ${result.requestId}"
    return result
}


// gets the address of the Hub
private getCallBackAddress() {
    return "http://" + device.hub.getDataValue("localIP") + ":" + device.hub.getDataValue("localSrvPortTCP")
}

// gets the address of the device. Address is stored as xxxxxxxx:yyyy where x is the IP address in hex and y is the port in hex
// Thus 192.168.1.140:8981 = C0A8018C:2315
private getHostAddress() {
    def ip = getDataValue("ip")
    def port = getDataValue("port")

    if (!ip || !port) {
        def parts = device.deviceNetworkId.split(":")
        if (parts.length == 2) {    
            ip = parts[0]
            port = parts[1]
        } else {
            log.warn "Can't figure out ip and port for device: ${device.id}"
        }
    }

    logDebug "Using IP: $ip and port: $port for device: ${device.id}"
    return convertHexToIP(ip) + ":" + convertHexToInt(port)
}

private Integer convertHexToInt(hex) {
    return Integer.parseInt(hex,16)
}

private String convertHexToIP(hex) {
    return [convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}

private logDebug( text ){
    // If debugging is enabled in settings, pass text to the logs
    
    if( settings.logDebug ) { 
        log.debug "${text}"
    }
}

private logDescriptionText( text ){
    // If description text is enabled, pass it as info to logs

    if( settings.logDescriptionText ) { 
        log.info "${text}"
    }
}

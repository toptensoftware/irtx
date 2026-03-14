const cmdSendIr = 1;
const cmdSendWol = 2;
const cmdHttpGet = 3;
const cmdHttpPost = 4;
const cmdUdpPacket = 5;
const cmdDelay = 6;
const cmdLed = 7;
const cmdSwitchActivity = 8;


let types = [

{
    name: "activitiesRoot",
    fields: [
        { name: "version", type: "int" },
        { name: "devices", type: "length" },
        { name: "devices", type: "device*" },
        { name: "activities", type: "length" },
        { name: "activities", type: "activity*" },
    ]
},

{
    name: "device",
    fields: [
        { name: "name", type: "string" },
        { name: "onCommands", type: "length" },
        { name: "onCommands", type: "command**" },
        { name: "offCommands", type: "length" },
        { name: "offCommands", type: "command**" },
    ]
},

{
    name: "activity",
    fields: [
        { name: "name", type: "string" },
        { name: "deviceStates", type: "length" },
        { name: "deviceStates", type: "deviceState*" },
        { name: "bindings", type: "length" },
        { name: "bindings", type: "binding*", default: [] },
        { name: "willActivateCommands", type: "length" },
        { name: "willActivateCommands", type: "command**", default: [] },
        { name: "didActivateCommands", type: "length" },
        { name: "didActivateCommands", type: "command**", default: [] },
        { name: "willDeactivateCommands", type: "length" },
        { name: "willDeactivateCommands", type: "command**", default: [] },
        { name: "didDectivateCommands", type: "length" },
        { name: "didDectivateCommands", type: "command**", default: [] },
    ]
},

{
    name: "command",
    resolveAbstractType: (val) => {
        switch (val.op)
        {
            case cmdSendIr: return "sendIrCommand";
            case cmdSendWol: return "sendWolCommand";
            case cmdHttpGet: return "httpGetCommand";
            case cmdHttpPost: return "httpPostCommand";
            case cmdUdpPacket: return "udpPacketCommand";
            case cmdDelay: return "delayCommand";
            case cmdLed: return "ledCommand";
            case cmdSwitchActivity: return "switchActivityCommand";
        }
    },
    fields: [
        { name: "op", type: "uint" },
    ]
},

{
    name: "sendIrCommand",
    baseType: "command",
    fields: [
        { name: "protocol", type: "uint" },
        { name: "irCode", type: "ulong" },
        { name: "ipAddr", type: "uint" },
    ]
},

{
    name: "sendWolCommand",
    baseType: "command",
    fields: [
        { name: "macaddr", type: "byte[6]" },
    ]
},

{
    name: "httpGetCommand",
    baseType: "command",
    fields: [
        { name: "url", type: "string" }
    ]
},

{
    name: "httpPostCommand",
    baseType: "command",
    fields: [
        { name: "url", type: "string" },
        { name: "data", type: "string" },
        { name: "contentType", type: "string" },
    ]
},

{
    name: "udpPacketCommand",
    baseType: "command",
    fields: [
        { name: "ipAddr", type: "uint" },
        { name: "data", type: "length" },
        { name: "data", type: "byte*" },
    ]
},

{
    name: "delayCommand",
    baseType: "command",
    fields: [
        { name: "duration", type: "uint" },
    ]
},

{
    name: "ledCommand",
    baseType: "command",
    fields: [
        { name: "color", type: "uint" },
        { name: "period", type: "uint" },
        { name: "duration", type: "uint" },
    ]
},

{
    name: "switchActivityCommand",
    baseType: "command",
    fields: [
        { name: "index", type: "uint" },
    ]
},

{
    name: "binding",
    fields: [
        { name: "protocol", type: "uint" },             // protocol name (riff)
        { name: "modifier", type: "ulong" },            // zero for non-modified
        { name: "value", type: "ulong" },               // ir code
        { name: "commands", type: "length" },
        { name: "commands", type: "command**" },       // commands to execute
    ]
},

{
    name: "deviceState",
    packMapper: (val, config) => {
        if (val.name !== undefined)
        {
            return {
                index: config.devices.findIndex(d => d.name === val.name),
                on: val.on,
            }
        }
        return val;
    },
    fields: [
        { name: "index", type: "int" },
        { name: "on", type: "bool" },
    ]
}

];


export default types;
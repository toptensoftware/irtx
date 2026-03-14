const cmdSendIr = 1;
const cmdSendWol = 2;
const cmdHttpGet = 3;
const cmdHttpPost = 4;
const cmdUdpPacket = 5;
const cmdDelay = 6;
const cmdLed = 7;
const cmdSwitchActivity = 8;

const bindingTypeIr = 1;


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
        { name: "onOps", type: "length" },
        { name: "onOps", type: "op**" },
        { name: "offOps", type: "length" },
        { name: "offOps", type: "op**" },
    ]
},

{
    name: "activity",
    fields: [
        { name: "name", type: "string" },
        { name: "devices", type: "length" },
        { name: "devices", type: "int*" },
        { name: "bindings", type: "length" },
        { name: "bindings", type: "binding**", default: [] },
        { name: "willActivateOps", type: "length" },
        { name: "willActivateOps", type: "op**", default: [] },
        { name: "didActivateOps", type: "length" },
        { name: "didActivateOps", type: "op**", default: [] },
        { name: "willDeactivateOps", type: "length" },
        { name: "willDeactivateOps", type: "op**", default: [] },
        { name: "didDectivateOps", type: "length" },
        { name: "didDectivateOps", type: "op**", default: [] },
    ]
},

{
    name: "op",
    resolveAbstractType: (val) => {
        switch (val.op)
        {
            case cmdSendIr: return "sendIrOp";
            case cmdSendWol: return "sendWolOp";
            case cmdHttpGet: return "httpGetOp";
            case cmdHttpPost: return "httpPostOp";
            case cmdUdpPacket: return "udpPacketOp";
            case cmdDelay: return "delayOp";
            case cmdLed: return "ledOp";
            case cmdSwitchActivity: return "switchActivityOp";
        }
    },
    fields: [
        { name: "op", type: "uint" },
    ]
},

{
    name: "sendIrOp",
    baseType: "op",
    fields: [
        { name: "protocol", type: "uint" },
        { name: "irCode", type: "ulong" },
        { name: "ipAddr", type: "uint" },
    ]
},

{
    name: "sendWolOp",
    baseType: "op",
    fields: [
        { name: "macaddr", type: "byte[6]" },
    ]
},

{
    name: "httpGetOp",
    baseType: "op",
    fields: [
        { name: "url", type: "string" }
    ]
},

{
    name: "httpPostOp",
    baseType: "op",
    fields: [
        { name: "url", type: "string" },
        { name: "data", type: "length" },
        { name: "data", type: "byte*" },
        { name: "contentType", type: "string" },
        { name: "contentEncoding", type: "string" },
    ]
},

{
    name: "udpPacketOp",
    baseType: "op",
    fields: [
        { name: "ipAddr", type: "uint" },
        { name: "data", type: "length" },
        { name: "data", type: "byte*" },
    ]
},

{
    name: "delayOp",
    baseType: "op",
    fields: [
        { name: "duration", type: "uint" },
    ]
},

{
    name: "ledOp",
    baseType: "op",
    fields: [
        { name: "color", type: "uint" },
        { name: "period", type: "uint" },
        { name: "duration", type: "uint" },
    ]
},

{
    name: "switchActivityOp",
    baseType: "op",
    fields: [
        { name: "index", type: "uint" },
    ]
},

{
    name: "binding",
    resolveAbstractType: (val) => {
        switch (val.type)
        {
            case bindingTypeIr: return "bindingIr";
        }
    },
    fields: [
        { name: "type", type: "uint" },
    ]
},

{
    name: "bindingIr",
    baseType: "binding",
    fields: [
        { name: "protocol", type: "uint" },             // protocol name (riff)
        { name: "modifier", type: "ulong" },            // zero for non-modified
        { name: "value", type: "ulong" },               // ir code
        { name: "ops", type: "length" },
        { name: "ops", type: "op**" },       // Ops to execute
    ]
},

];


export default types;
import { registerEnum } from "@toptensoftware/binpack";

const opId = {
    send_ir:         1,
    send_wol:        2,
    http_get:        3,
    http_post:       4,
    udp_packet:      5,
    delay:           6,
    led:             7,
    switch_activity: 8,
    set_ir_reg:      9,
    search_string:   10,
    if_true:         11,
};
registerEnum("op", opId);

const bindingType = {
    ir:     1,
    ir_any: 2,
};
registerEnum("binding_type", bindingType);

const bindingFlags = {
    continue_routing: 1,
};
registerEnum("binding_flags", bindingFlags);


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
            case opId.send_ir:         return "sendIrOp";
            case opId.send_wol:        return "sendWolOp";
            case opId.http_get:        return "httpGetOp";
            case opId.http_post:       return "httpPostOp";
            case opId.udp_packet:      return "udpPacketOp";
            case opId.delay:           return "delayOp";
            case opId.led:             return "ledOp";
            case opId.switch_activity: return "switchActivityOp";
            case opId.set_ir_reg:      return "setIrRegOp";
            case opId.search_string:   return "searchStringOp";
            case opId.if_true:         return "ifTrueOp";
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
    name: "setIrRegOp",
    baseType: "op",
    fields: [
        { name: "protocol", type: "uint" },
        { name: "irCode",   type: "ulong" },
    ]
},

{
    name: "searchStringOp",
    baseType: "op",
    fields: [
        { name: "matchString", type: "string" },
    ]
},

{
    name: "ifTrueOp",
    baseType: "op",
    fields: [
        { name: "trueOps",  type: "length" },
        { name: "trueOps",  type: "op**" },
        { name: "falseOps", type: "length" },
        { name: "falseOps", type: "op**" },
    ]
},

{
    name: "binding",
    resolveAbstractType: (val) => {
        switch (val.type)
        {
            case bindingType.ir:     return "bindingIr";
            case bindingType.ir_any: return "bindingIrAny";
        }
    },
    fields: [
        { name: "type", type: "uint" },
        { name: "flags", type: "uint", default: 0 },
        { name: "ops", type: "length" },
        { name: "ops", type: "op**" },       // Ops to execute
    ]
},

{
    name: "bindingIr",
    baseType: "binding",
    fields: [
        { name: "protocol", type: "uint" },             // protocol name (riff)
        { name: "modifier", type: "ulong" },            // zero for non-modified
        { name: "value", type: "ulong" },               // ir code
    ]
},

{
    name: "bindingIrAny",
    baseType: "binding",
    fields: []                                          // matches any received IR code
},

];


export default types;
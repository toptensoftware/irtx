export const opId = {
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
    wait_http:       12,
};

export const bindingType = {
    ir:     1,
    ir_any: 2,
};

export const bindingFlags = {
    continue_routing: 1,
};

export const irEventKindMask = {
    press:      1,
    repeat:     2,
    long_press: 4,
    release:    8,
};



/**
 * Encodes a 4-character ASCII string as a little-endian 32-bit RIFF FourCC integer.
 * @param {string} a - A string of up to 4 characters (padded with spaces if shorter).
 * @returns {number} The FourCC value as a 32-bit integer.
 */
export function riff(a)
{
    if (typeof(a) === "string")
    {
        while (a.length < 4)
            a = a + " ";

        return (a.charCodeAt(0)) |
            (a.charCodeAt(1) << 8) |
            (a.charCodeAt(2) << 16) |
            (a.charCodeAt(3) << 24);
    }
    throw new Error("Invalid riff value");
}


let types = [

{ name: "op", enum: opId },
{ name: "binding_type", enum: bindingType },
{ name: "binding_flags", enum: bindingFlags },
{ name: "ir_event_kind_mask", enum: irEventKindMask },

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
    packMapper: (value) => {
        if (typeof value == "string")
        {
            if (value === "*")
                return { op: opId.send_ir, protocol: 0, irCode: 0n }
            if (value.startsWith("http://"))
                return { op: opId.http_get, url: value }

            let m = value.match(/^([A-Z0-9]+):(?:0x)?([a-fA-F0-9]+)?/);
            return { op: opId.send_ir, protocol: riff(m[1]), irCode: BigInt("0x" + m[2])}

        }
        return value;
    },
    resolveVirtualType: (val) => {
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
            case opId.wait_http:       return "waitHttpOp";
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
        { name: "ipAddr", type: "uint", default: 0 },
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
    name: "waitHttpOp",
    baseType: "op",
    fields: []
},

{
    name: "binding",
    packMapper: (value) => {

        // Map string args
        value = Object.assign({}, value);
        if (value.on)
        {
            if (value.on === "*")
            {
                // Any IR Code
                value = { type: bindingType.ir_any , ...value };
            }
            else
            {
                // <protocol>:<modifier>+<ircode>
                let m = value.on.match(/^([A-Z0-9]+):(?:0x)?(?:([a-fA-F0-9]+)\+)?(?:0x)?([a-fA-F0-9]+)?/);
                value.type = bindingType.ir;
                value.protocol = riff(m[1]);
                value.modifier = m[2] ? BigInt("0x" + m[2]) : 0n;
                value.value = BigInt("0x" + m[3]);
            }

            delete value.on;
        }
        if (value.op)
        {
            value.ops = [ value.op ];
            delete value.op;
        }
        if (!Array.isArray(value.ops))
            value.ops = [ value.ops ];
        return value;
    },
    resolveVirtualType: (val) => {
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
        { name: "protocol",    type: "uint" },                                  // protocol name (riff)
        { name: "eventMask",   type: "uint", default: irEventKindMask.press },  // bitmask of IrEventKind values to match (0xF = all)
        { name: "modifier",    type: "ulong" },                                 // zero for non-modified
        { name: "value",       type: "ulong" },                                 // ir code
        { name: "minHoldTime",    type: "uint", default: 0 },                  // minimum hold time in ms (0 = fire immediately)
        { name: "repeatRate", type: "uint", default: 0 },                  // synthetic repeat interval in ms (0 = use natural IR repeat rate)
    ]
},

{
    name: "bindingIrAny",
    baseType: "binding",
    fields: []                                          // matches any received IR code
},

];


export default types;
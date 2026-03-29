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
    gpio:   3,
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

    if (typeof(a) === "number")
        return a;

    throw new Error("Invalid riff value");
}

/**
 * Parses an IPv4 address into a 32-bit value
 * @param {string} ip the ip address to parse
 * @returns {Number} packed ip address
 */
export function parseIPv4(ip) {
    if (typeof ip == "string")
        return ip.split('.').reverse().reduce((acc, octet) => (acc << 8) | parseInt(octet, 10), 0) >>> 0;
    else
        return ip;
}

/**
 * 
 * @param {String} macaddr the mac address to parse 
 * @returns {Number[]} mac address as 6 integers
 */
export function parseMacAddress(ip) {
    if (typeof ip == "string")
    {
        let parts = ip.split(':');
        if (parts.length != 6)
            throw new Error("Invalid mac address - should have 6 parts");

        return parts.map(x => parseInt("0x" + x));
    }
    else
        return ip;
}

export class op
{
    static switchActivity(nameOrIndex)
    {
        return { 
            op: opId.switch_activity,
            index: nameOrIndex,
        }
    }

    static httpGet(url)
    {
        return {
            op: opId.http_get,
            url,
        }
    }

    static sendWol(macaddr)
    {
        return {
            op: opId.send_wol,
            macaddr: parseMacAddress(macaddr)
        }
    }

    static sendIr(irCode, ipAddr = 0)
    {
        let m = irCode.match(/^([A-Z0-9]+):(?:0x)?([a-fA-F0-9]+)?/);
        if (!m)
            throw new Error("Invalid irCode");

        return {
            op: opId.send_ir,
            protocol: riff(m[1]),
            irCode: BigInt("0x" + m[2]),
            ipAddr: parseIPv4(ipAddr),
        }
    }
}

function makeArray(value)
{
    if (value == null || value == undefined)
        return value;
    if (Array.isArray(value))
        return value;
    return [ value ];
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
    packMapper: (value) =>
    {
        value.turnOn = makeArray(value.turnOn);
        value.turnOff = makeArray(value.turnOff);
        return value;
    },
    fields: [
        { name: "name", type: "string" },
        { name: "turnOn", type: "length" },
        { name: "turnOn", type: "op**", default: [] },
        { name: "turnOff", type: "length" },
        { name: "turnOff", type: "op**", default: [] },
    ]
},

{
    name: "activity",
    packMapper: (value, root) => {
        if (Array.isArray(value.devices))
        {
            value.devices = value.devices.map((name) => {
                if (typeof name == 'string')
                {
                    let index = root.devices.findIndex(x => x.name == name);
                    if (index < 0)
                        throw new Error(`Unknown device ${name}`);
                    return index;
                }
                return name;
            });
        }
        value.willActivate = makeArray(value.willActivate);
        value.willDeactivate = makeArray(value.willDeactivate);
        value.didActivate = makeArray(value.didActivate);
        value.didDeactivate = makeArray(value.didDeactivate);
        return value;
    },
    fields: [
        { name: "name", type: "string" },
        { name: "devices", type: "length" },
        { name: "devices", type: "int*" },
        { name: "bindings", type: "length" },
        { name: "bindings", type: "binding**", default: [] },
        { name: "willActivate", type: "length" },
        { name: "willActivate", type: "op**", default: [] },
        { name: "didActivate", type: "length" },
        { name: "didActivate", type: "op**", default: [] },
        { name: "willDeactivate", type: "length" },
        { name: "willDeactivate", type: "op**", default: [] },
        { name: "didDeactivate", type: "length" },
        { name: "didDeactivate", type: "op**", default: [] },
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
        { name: "protocol", type: "uint", packMapper: riff },
        { name: "irCode", type: "ulong" },
        { name: "ipAddr", type: "uint", default: 0, packMapper: parseIPv4 },
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
    packMapper: (value, root) => 
    {
        if (typeof value.index === 'string')
        {
            let name = value.index;
            value.index = root.activities.findIndex(x => x.name == value.index);
            if (value.index < 0)
                throw new Error(`Unknown activity ${name}`);
        }
        return value;
    },
    fields: [
        { name: "index", type: "uint" },
    ]
},

{
    name: "setIrRegOp",
    baseType: "op",
    fields: [
        { name: "protocol", type: "uint", packMapper: riff },
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
            else if (value.on.startsWith("gpio:"))
            {
                // gpio:<pin>
                value.type = bindingType.gpio;
                value.pin = parseInt(value.on.slice(5));
            }
            else
            {
                // <protocol>:<modifier>+<ircode>
                let m = value.on.match(/^([A-Z0-9]+):(?:0x)?(?:([a-fA-F0-9]+)\+)?(?:0x)?([a-fA-F0-9]+)?/);
                value.type = bindingType.ir;
                value.protocol = riff(m[1]);
                if (m[2])
                    value.modifier = BigInt("0x" + m[2]);
                value.value = BigInt("0x" + m[3]);
            }

            delete value.on;
        }

        // Crack protocol:ircode modifier from string
        if (typeof value.modifier === 'string')
        {
            let m = value.modifier.match(/^([A-Z0-9]+):(?:0x)?([a-fA-F0-9]+)?/);
            let modifierProtocol = riff(m[1]);
            if (value.protocol && value.protocol != modifierProtocol)
                throw new Error(`Modifier has mismatched protocol ${value.protocol} != ${modifierProtocol}`);
            value.modifier = BigInt("0x" + m[2]);
        }

        value.do = makeArray(value.do);
        return value;
    },
    resolveVirtualType: (val) => {
        switch (val.type)
        {
            case bindingType.ir:     return "bindingIr";
            case bindingType.ir_any: return "bindingIrAny";
            case bindingType.gpio:   return "bindingGpio";
        }
    },
    fields: [
        { name: "type", type: "uint" },
        { name: "flags", type: "uint", default: 0 },
        { name: "do", type: "length", cname: "doOps_count" },
        { name: "do", type: "op**", cname: "doOps" },       // Ops to execute
    ]
},

{
    name: "bindingIr",
    baseType: "binding",
    fields: [
        { name: "protocol",    type: "uint", packMapper: riff },                // protocol name (riff)
        { name: "eventMask",   type: "uint", default: irEventKindMask.press },  // bitmask of IrEventKind values to match (0xF = all)
        { name: "modifier",    type: "ulong", default: 0 },                                 // zero for non-modified
        { name: "value",       type: "ulong" },                                 // ir code
        { name: "minHoldTime", type: "uint", default: 0 },                   // minimum hold time in ms (0 = fire immediately)
        { name: "repeatRate",  type: "uint", default: 0 },                       // synthetic repeat interval in ms (0 = use natural IR repeat rate)
    ]
},

{
    name: "bindingIrAny",
    baseType: "binding",
    fields: []                                          // matches any received IR code
},

{
    name: "bindingGpio",
    baseType: "binding",
    fields: [
        { name: "pin",          type: "uint" },                                              // GPIO pin number
        { name: "eventMask",    type: "uint", default: irEventKindMask.press },              // press=1, repeat=2, release=8
        { name: "minHoldTime",  type: "uint", default: 0 },                                  // ms held before event fires (0 = immediate)
        { name: "initialDelay", type: "uint", default: 0 },                                  // ms after press before first repeat (0 = use repeatRate)
        { name: "repeatRate",   type: "uint", default: 0 },                                  // repeat interval in ms (0 = no repeat)
    ]
},

];


export default types;
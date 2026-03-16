import { bindingType, opId } from "./binpack.js";

const activitiesRoot = {
    version: 1,
    devices: [
        { 
            name: "tv", 
            onOps: [], 
            offOps: [] 
        },
        { 
            name: "receiver", 
            onOps: [], 
            offOps: [] 
        },
        { 
            name: "dvr", 
            onOps: [], 
            offOps: [] 
        },
        { 
            name: "atv", 
            onOps: [], 
            offOps: [] 
        },
    ],
    activities: [
        {
            name: "off",
            devices: [ ],
            bindings: [
                {
                    type: bindingType.ir,
                    protocol: 0x414E4150,
                    modifier: 0x000040040D08BCB9,
                    value: 0x000040040D08080D,
                    ops: [
                        { op: opId.http_get, url: "http://10.1.1.125:3000/act1" },
                    ]
                },
                {
                    type: bindingType.ir,
                    protocol: 0x414E4150,
                    modifier: 0x000040040D08BCB9,
                    value: 0x000040040D08888D,
                    ops: [
                        { op: opId.http_get, url: "http://10.1.1.125:3000/act2" },
                    ]
                },
                { 
                    type: bindingType.ir_any,
                    ops: [
                        { op: opId.send_ir, protocol: 0, irCode: 0n }
                    ]
                },
            ]
        }
    ],
};

export default activitiesRoot;

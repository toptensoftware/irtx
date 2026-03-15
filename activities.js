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

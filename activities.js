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
                    // Power + btn 1
                    on: "PANA:40040D08BCB9+40040D08080D",
                    op: "http://10.1.1.125:3000/act1",
                },

                {
                    // Power + btn 2
                    on: "PANA:40040D08BCB9+40040D08888D",
                    op: "http://10.1.1.125:3000/act2",
                },

                {
                    // Volume up
                    on: "PANA:0000400401000405",
                    eventMask: 0x03,        // enable repeat
                    op: "http://"
                },

                {
                    // Volume down
                    on: "PANA:0000400401008485",
                    eventMask: 0x03,        // enable repeat
                    op: "http://"
                },

                {
                    // Pass through all other IR Codes
                    on: "*",
                    op: "*",
                }
            ]
        }
    ],
};

export default activitiesRoot;

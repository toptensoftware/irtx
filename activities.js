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
                    // Volume up x5
                    on: "PANA:0000400401000405",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 2500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=up?step=5",
                },

                {
                    // Volume down x5
                    on: "PANA:0000400401008485",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 2500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=down?step=5",
                },

                {
                    // Volume up x2
                    on: "PANA:0000400401000405",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 1500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=up?step=3",
                },

                {
                    // Volume down x2
                    on: "PANA:0000400401008485",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 1500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=down?step=3",
                },

                {
                    // Volume up x1 after initial delay
                    on: "PANA:0000400401000405",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=up",
                },

                {
                    // Volume down x1 after initial delay
                    on: "PANA:0000400401008485",
                    eventMask: 0x02,        // repeat only
                    minHoldTime: 500,
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=down",
                },

                {
                    // Volume up (consume)
                    on: "PANA:0000400401000405",
                    eventMask: 0x02,        // repeat only
                    ops: [],
                },

                {
                    // Volume down (consume)
                    on: "PANA:0000400401008485",
                    eventMask: 0x02,        // repeat only
                    ops: [],
                },

                {
                    // Volume up x1
                    on: "PANA:0000400401000405",
                    ops: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=up",
                    repeatRate: 60,
                },

                {
                    // Volume down x1
                    on: "PANA:0000400401008485",
                    op: "http://10.1.1.125:3000/YamahaExtendedControl/v1/main/setVolume?volume=down",
                    repeatRate: 60,
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


/*
volume gradient

press - adjust by 0.5
from 0.5 to 1.5 seconds from press adjust by 1.5 dB every 0.6s
from 1.5 seconds onwards adjust by 2.5 dB every 0.6s


*/

import { opId } from "./binpack.js";

const yamaUrl = "http://10.1.1.103/YamahaExtendedControl/v1";

const activitySwitchControls = [
    {
        // Power + btn 1
        on: "PANA:40040D08080D",
        op: {
            op: opId.switch_activity,
            index: 1,
        }
    },

    {
        // Power + btn 2
        on: "PANA:40040D08888D",
        op: {
            op: opId.switch_activity,
            index: 2,
        }
    },

    {
        // Power + btn 3
        on: "PANA:40040D08484D",
        op: {
            op: opId.switch_activity,
            index: 3,
        }
    },

    {
        // Power + btn 4
        on: "PANA:40040D08C8CD",
        op: {
            op: opId.switch_activity,
            index: 4,
        }
    },

    {
        // Power + btn 0
        on: "PANA:40040D08989D",
        op: {
            op: opId.switch_activity,
            index: 0,
        }
    },
];


/*
implements an accelerating volume gradient based on the default
yamahe remote volume adjustment

- on press adjust by 0.5 dB
- after held for 0.5 adjust by 1.5 dB every 0.6s
- after held for 1.5 seconds adjust by 2.5 dB every 0.6s
*/
const volumeControls = [
    {
        // Volume up x5
        on: "PANA:0000400401000405",
        eventMask: 0x02,        // repeat only
        minHoldTime: 2500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=up&step=5",
    },

    {
        // Volume down x5
        on: "PANA:0000400401008485",
        eventMask: 0x02,        // repeat only
        minHoldTime: 2500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=down&step=5",
    },

    {
        // Volume up x2
        on: "PANA:0000400401000405",
        eventMask: 0x02,        // repeat only
        minHoldTime: 1500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=up&step=3",
    },

    {
        // Volume down x2
        on: "PANA:0000400401008485",
        eventMask: 0x02,        // repeat only
        minHoldTime: 1500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=down&step=3",
    },

    {
        // Volume up x1 after initial delay
        on: "PANA:0000400401000405",
        eventMask: 0x02,        // repeat only
        minHoldTime: 500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=up",
    },

    {
        // Volume down x1 after initial delay
        on: "PANA:0000400401008485",
        eventMask: 0x02,        // repeat only
        minHoldTime: 500,
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=down",
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
        ops: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=up",
        repeatRate: 60,
    },

    {
        // Volume down x1
        on: "PANA:0000400401008485",
        op: "http://10.1.1.103/YamahaExtendedControl/v1/main/setVolume?volume=down",
        repeatRate: 60,
    },
]

const activitiesRoot = {
    version: 1,
    devices: [
        { 
            name: "tv", 
            onOps: [
                {
                    op: opId.send_wol,
                    macaddr: [ 0x44, 0xcb, 0x8b, 0x0c, 0xd5, 0x56 ],
                }
            ], 
            offOps: [
                {
                    op: opId.send_ir,
                    protocol: 0x2043454E,
                    irCode: 0x20dfa35c,
                    ipAddr: 0xBB01010A,
                }
            ] 
        },
        { 
            name: "receiver", 
            onOps: [
                {
                    op: opId.http_get,
                    url: yamaUrl + "/main/setPower?power=on"
                }
            ], 
            offOps: [
                {
                    op: opId.http_get,
                    url: yamaUrl + "/main/setPower?power=standby"
                }
            ] 
        },
        { 
            name: "dvr", 
            onOps: [], 
            offOps: [] 
        },
        { 
            name: "atv", 
            onOps: [
                {
                    op: opId.send_ir,
                    protocol: 0x2043454E,
                    irCode: 0xa7e1347f,
                    ipAddr: 0xBB01010A,
                }
            ], 
            offOps: [
                {
                    op: opId.send_ir,
                    protocol: 0x2043454E,
                    irCode: 0xa7e1547f,
                    ipAddr: 0xBB01010A,
                }
            ] 
        },
    ],
    activities: [
        {
            name: "off",
            devices: [ ],
            bindings: [
                ...activitySwitchControls,
            ]
        },

        {
            name: "hdd",
            devices: [ 0, 1, 2 ],
            bindings: [
                ...activitySwitchControls,
                ...volumeControls,
                {
                    // Pass through all other IR Codes
                    on: "*",
                    op: "*",
                }
            ],
            didActivateOps: [
                {
                    op: opId.http_get,
                    url: yamaUrl + "/main/setInput?input=hdmi1"
                },
            ],
        },

        {
            name: "atv",
            devices: [ 0, 1, 3 ],
            bindings: [
                ...activitySwitchControls,
                ...volumeControls,
            ],
            didActivateOps: [
                {
                    op: opId.http_get,
                    url: yamaUrl + "/main/setInput?input=hdmi2"
                },
            ],
        }
    ],
};

export default activitiesRoot;



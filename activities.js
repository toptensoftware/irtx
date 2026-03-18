
import { op, opId } from "./binpack.js";
import ircodes from './ircodes.json' with { type: 'json' }

/*
// URL for yamaha receiver
const yamaUrl = "http://10.1.1.103/YamahaExtendedControl/v1";

// IP address of remote IR blaster
const blasterIp = "10.1.1.187";

// Mac address of LG TV
const lgMacAddr = "44:cb:8b:0c:d5:56";
*/

// URL for yamaha receiver
const yamaUrl = "http://10.1.1.125:3000/YamahaExtendedControl/v1";

// IP address of remote IR blaster
const blasterIp = "10.1.1.125";

// Mac address of LG TV
const lgMacAddr = "00:11:22:33:44:55";


// Common bindings for switching between activities
const activitySwitchControls = [
    {
        // Power + btn 1
        on: ircodes.pana.btn1,
        modifier: ircodes.pana.power,
        do: op.switchActivity("dvr"),
    },

    {
        // Power + btn 2
        on: ircodes.pana.btn2,
        modifier: ircodes.pana.power,
        do: op.switchActivity("atv"),
    },

    {
        // Power + btn 0
        on: ircodes.pana.btn0,
        modifier: ircodes.pana.power,
        do: op.switchActivity("off"),
    },

    {
        on: "PANA:40040D000805",
        modifier: "PANA:40040D00BCB1",
        do: op.httpGet("http://10.1.1.125:3000/powerButton")
    }
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
        on: ircodes.pana.tvVolumeUp,
        eventMask: 0x02,        // repeat only
        minHoldTime: 2500,
        do: op.httpGet(yamaUrl +"/main/setVolume?volume=up&step=5"),
    },

    {
        // Volume down x5
        on: ircodes.pana.tvVolumeDown,
        eventMask: 0x02,        // repeat only
        minHoldTime: 2500,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=down&step=5"),
    },

    {
        // Volume up x2
        on: ircodes.pana.tvVolumeUp,
        eventMask: 0x02,        // repeat only
        minHoldTime: 1500,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=up&step=3"),
    },

    {
        // Volume down x2
        on: ircodes.pana.tvVolumeDown,
        eventMask: 0x02,        // repeat only
        minHoldTime: 1500,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=down&step=3"),
    },

    {
        // Volume up x1 after initial delay
        on: ircodes.pana.tvVolumeUp,
        eventMask: 0x02,        // repeat only
        minHoldTime: 500,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=up"),
    },

    {
        // Volume down x1 after initial delay
        on: ircodes.pana.tvVolumeDown,
        eventMask: 0x02,        // repeat only
        minHoldTime: 500,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=down"),
    },

    {
        // Volume up (consume)
        on: ircodes.pana.tvVolumeUp,
        eventMask: 0x02,        // repeat only
        do: [ /* no-op */ ],
    },

    {
        // Volume down (consume)
        on: ircodes.pana.tvVolumeDown,
        eventMask: 0x02,        // repeat only
        do: [ /* no-op */ ],
    },

    {
        // Volume up x1
        on: ircodes.pana.tvVolumeUp,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=up"),
        repeatRate: 60,
    },

    {
        // Volume down x1
        on: ircodes.pana.tvVolumeDown,
        do: op.httpGet(yamaUrl + "/main/setVolume?volume=down"),
        repeatRate: 60,
    },
]

const activitiesRoot = {
    version: 1,
    devices: [
        { 
            name: "tv", 
            turnOn: op.sendWol(lgMacAddr),
            turnOff: op.sendIr(ircodes.lg.powerOff, blasterIp),
        },
        { 
            name: "receiver", 
            turnOn: op.httpGet(yamaUrl + "/main/setPower?power=on"),
            turnOff: op.httpGet(yamaUrl + "/main/setPower?power=standby"),
        },
        { 
            name: "dvr", 
        },
        { 
            name: "atv", 
            turnOn: op.sendIr("NEC:a7e1347f", blasterIp),
            turnOff: op.sendIr("NEC:a7e1547f", blasterIp),
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
            name: "dvr",
            devices: [ "tv", "receiver", "dvr" ],
            bindings: [
                ...activitySwitchControls,
                ...volumeControls,
                {
                    // Pass through all other IR Codes
                    on: "*",
                    do: "*",
                }
            ],
            didActivate: op.httpGet(yamaUrl + "/main/setInput?input=hdmi1"),
        },

        {
            name: "atv",
            devices: [ "tv", "receiver", "atv" ],
            bindings: [
                ...activitySwitchControls,
                ...volumeControls,
                {
                    on: ircodes.pana.up,
                    do: op.sendIr(ircodes.atv.up, blasterIp)
                },
                {
                    on: ircodes.pana.down,
                    do: op.sendIr(ircodes.atv.down, blasterIp)
                },
                {
                    on: ircodes.pana.left,
                    do: op.sendIr(ircodes.atv.left, blasterIp)
                },
                {
                    on: ircodes.pana.right,
                    do: op.sendIr(ircodes.atv.right, blasterIp)
                },
                {
                    on: ircodes.pana.ok,
                    do: op.sendIr(ircodes.atv.select, blasterIp)
                },
                {
                    on: ircodes.pana.exit,
                    do: op.sendIr(ircodes.atv.home, blasterIp)
                },
                {
                    on: ircodes.pana.return,
                    do: op.sendIr(ircodes.atv.menu, blasterIp)
                },
                {
                    on: ircodes.pana.nextTrack,
                    do: op.sendIr(ircodes.atv.next, blasterIp)
                },
                {
                    on: ircodes.pana.previousTrack,
                    do: op.sendIr(ircodes.atv.previous, blasterIp)
                },
                {
                    on: ircodes.pana.forward,
                    do: op.sendIr(ircodes.atv.forward10x, blasterIp)
                },
                {
                    on: ircodes.pana.reverse,
                    do: op.sendIr(ircodes.atv.reverse10x, blasterIp)
                },
                {
                    on: ircodes.pana.play,
                    do: op.sendIr(ircodes.atv.play, blasterIp)
                },
                {
                    on: ircodes.pana.pause,
                    do: op.sendIr(ircodes.atv.pause, blasterIp)
                },
                {
                    on: ircodes.pana.stop,
                    do: op.sendIr(ircodes.atv.menu, blasterIp)
                },
                {
                    on: ircodes.pana.skipForward60,
                    do: op.sendIr(ircodes.atv.skipForward10, blasterIp)
                },
                {
                    on: ircodes.pana.skipBack10,
                    do: op.sendIr(ircodes.atv.skipBack10, blasterIp)
                },
            ],
            didActivate: op.httpGet(yamaUrl + "/main/setInput?input=hdmi2"),
        }
    ],
};

export default activitiesRoot;



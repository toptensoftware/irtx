import { Component, css, router, $ } from "@codeonlyjs/core";
import { config } from "./config.js";

export class HomePage extends Component
{
    status = null;

    onMount()
    {
        this.load(async () => {
            let r = await fetch(`${config.deviceUrl}/status`);
            if (!r.ok)
                throw new Error(`HTTP ${r.status}`);
            this.status = await r.json();
        });
    }

    formatPins(pins)
    {
        if (!pins || pins.length === 0)
            return "none";
        return pins.map(p => Array.isArray(p) ? `encoder(${p[0]}, ${p[1]})` : String(p)).join(", ");
    }

    static template = {
        type: "main",
        class: "home-page",
        $: [
            {
                if: c => c.loading,
                type: "div",
                class: "status-loading",
                text: "Loading…",
            },
            {
                elseif: c => c.loadError,
                type: "div",
                class: "status-error",
                text: c => `Failed to load status: ${c.loadError.message}`,
            },
            {
                elseif: c => c.status,
                type: "div",
                class: "status-sections",
                $: [
                    // Device
                    $.h2("Device"),
                    {
                        type: "table",
                        class: "kv-table",
                        $: [
                            { type: "tr", $: [$.th("Name"),     { type: "td", text: c => c.status.device.name }] },
                            { type: "tr", $: [$.th("Logging"),  { type: "td", text: c => c.status.device.logging }] },
                        ]
                    },

                    // WiFi
                    $.h2("WiFi"),
                    {
                        type: "table",
                        class: "kv-table",
                        $: [
                            { type: "tr", $: [$.th("Mode"),      { type: "td", text: c => c.status.wifi.mode }] },
                            { type: "tr", $: [$.th("SSID"),      { type: "td", text: c => c.status.wifi.ssid }] },
                            { type: "tr", $: [$.th("Connected"), { type: "td", text: c => c.status.wifi.connected ? "Yes" : "No" }] },
                            { type: "tr", $: [$.th("IP"),        { type: "td", text: c => c.status.wifi.ip }] },
                            { type: "tr", $: [$.th("Gateway"),   { type: "td", text: c => c.status.wifi.gateway }] },
                            { type: "tr", $: [$.th("RSSI"),      { type: "td", text: c => `${c.status.wifi.rssi} dBm` }] },
                            { type: "tr", $: [$.th("MAC"),       { type: "td", text: c => c.status.wifi.mac }] },
                            { type: "tr", $: [$.th("UDP Port"),  { type: "td", text: c => String(c.status.wifi.udp_port) }] },
                        ]
                    },

                    // GPIO
                    $.h2("GPIO"),
                    {
                        type: "table",
                        class: "kv-table",
                        $: [
                            { type: "tr", $: [$.th("IR TX Pin"),  { type: "td", text: c => String(c.status.gpio.irtx_pin) }] },
                            { type: "tr", $: [$.th("IR RX Pin"),  { type: "td", text: c => c.status.gpio.irrx_pin === -1 ? "disabled" : String(c.status.gpio.irrx_pin) }] },
                            { type: "tr", $: [$.th("LED Pin"),    { type: "td", text: c => String(c.status.gpio.led_pin) }] },
                            { type: "tr", $: [$.th("LED Order"),  { type: "td", text: c => c.status.gpio.led_order }] },
                            { type: "tr", $: [$.th("Pullup"),     { type: "td", text: c => c.formatPins(c.status.gpio.pullup) }] },
                            { type: "tr", $: [$.th("Pulldown"),   { type: "td", text: c => c.formatPins(c.status.gpio.pulldown) }] },
                        ]
                    },

                    // Protocols
                    $.h2("Protocols"),
                    {
                        type: "table",
                        class: "data-table",
                        $: [
                            {
                                type: "thead",
                                $: { type: "tr", $: [$.th("Name"), $.th("ID"), $.th("Bits")] }
                            },
                            {
                                type: "tbody",
                                $: {
                                    foreach: c => c.status.protocols,
                                    type: "tr",
                                    $: [
                                        { type: "td", text: p => p.name },
                                        { type: "td", text: p => p.id },
                                        { type: "td", text: p => String(p.bits) },
                                    ]
                                }
                            }
                        ]
                    },

                    // BLE
                    $.h2("BLE"),
                    {
                        type: "table",
                        class: "data-table",
                        $: [
                            {
                                type: "thead",
                                $: { type: "tr", $: [$.th("Slot"), $.th("ID"), $.th("Peer")] }
                            },
                            {
                                type: "tbody",
                                $: {
                                    foreach: c => c.status.ble,
                                    type: "tr",
                                    $: [
                                        { type: "td", text: b => String(b.slot) },
                                        { type: "td", text: b => b.id },
                                        { type: "td", text: b => b.peer ?? "—" },
                                    ]
                                }
                            }
                        ]
                    },

                    // Activities
                    $.h2("Activities"),
                    {
                        type: "table",
                        class: "kv-table",
                        $: [
                            { type: "tr", $: [$.th("Queue"),   { type: "td", text: c => c.status.activities.queue }] },
                            { type: "tr", $: [$.th("Devices"), { type: "td", text: c => c.status.activities.devices.length === 0 ? "none" : c.status.activities.devices.join(", ") }] },
                        ]
                    },
                    {
                        type: "table",
                        class: "data-table",
                        $: [
                            {
                                type: "thead",
                                $: { type: "tr", $: [$.th("#"), $.th("Name"), $.th("Active")] }
                            },
                            {
                                type: "tbody",
                                $: {
                                    foreach: c => c.status.activities.list,
                                    type: "tr",
                                    $: [
                                        { type: "td", text: a => String(a.index) },
                                        { type: "td", text: a => a.name },
                                        { type: "td", text: a => a.active ? "✓" : "" },
                                    ]
                                }
                            }
                        ]
                    },
                ]
            }
        ]
    }
}

css`
.home-page
{
    padding: 20px 30px;
    max-width: 800px;

    h2
    {
        margin: 1.5rem 0 0.4rem;
        font-size: 0.8rem;
        text-transform: uppercase;
        letter-spacing: 0.08em;
        opacity: 0.6;
        border-bottom: 1px solid var(--gridline-color);
        padding-bottom: 0.25rem;
    }

    .kv-table,
    .data-table
    {
        width: 100%;
        border-collapse: collapse;
        font-size: 0.9rem;

        th, td
        {
            padding: 4px 12px 4px 0;
            text-align: left;
            border-bottom: 1px solid var(--gridline-color);
        }
    }

    .kv-table
    {
        th
        {
            font-weight: normal;
            opacity: 0.6;
            white-space: nowrap;
            width: 1%;
            padding-right: 24px;
        }
    }

    .data-table
    {
        thead th
        {
            font-weight: 600;
            padding-right: 24px;
        }

        td
        {
            padding-right: 24px;
        }
    }

    .status-loading
    {
        margin-top: 3rem;
        opacity: 0.5;
    }

    .status-error
    {
        margin-top: 2rem;
        color: var(--danger-color, #c00);
    }
}
`

router.register({
    pattern: "/",
    match: (to) => {
        to.page = new HomePage();
        return true;
    },
});

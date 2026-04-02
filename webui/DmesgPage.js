import { Component, css, router } from "@codeonlyjs/core";
import { config } from "./config.js";

export class DmesgPage extends Component
{
    log = null;

    onMount()
    {
        this.load(async () => {
            let r = await fetch(`${config.deviceUrl}/api/dmesg`);
            if (!r.ok)
                throw new Error(`HTTP ${r.status}`);
            this.log = await r.text();
        });
    }

    static template = {
        type: "main",
        class: "dmesg-page",
        $: [
            {
                if: c => c.loading,
                type: "div",
                class: "dmesg-loading",
                text: "Loading…",
            },
            {
                elseif: c => c.loadError,
                type: "div",
                class: "dmesg-error",
                text: c => `Failed to load log: ${c.loadError.message}`,
            },
            {
                elseif: c => c.log !== null,
                type: "pre",
                class: "dmesg-output",
                text: c => c.log,
            },
        ]
    }
}

css`
.dmesg-page
{
    padding: 20px 30px;

    .dmesg-output
    {
        font-family: monospace;
        font-size: 0.85rem;
        white-space: pre-wrap;
        word-break: break-all;
        margin: 0;
    }

    .dmesg-loading
    {
        opacity: 0.5;
    }

    .dmesg-error
    {
        color: var(--danger-color, #c00);
    }
}
`

router.register({
    pattern: "/dmesg",
    match: (to) => {
        to.page = new DmesgPage();
        return true;
    },
});

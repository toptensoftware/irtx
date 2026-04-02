import { Component, css, router } from "@codeonlyjs/core";
import { config } from "./config.js";

class ConsolePage extends Component
{
    entries = [];
    busy = false;

    async runCommand(cmd)
    {
        this.entries.push({ kind: "command", text: cmd });
        this.busy = true;
        this.update();
        this.outputEl.scrollTop = this.outputEl.scrollHeight;

        try
        {
            let r = await fetch(`${config.deviceUrl}/api/command`, {
                method: "POST",
                body: cmd,
            });
            let text = await r.text();
            if (!r.ok)
                this.entries.push({ kind: "error", text: `Error ${r.status}: ${text}` });
            else
                this.entries.push({ kind: "response", text });
        }
        catch (e)
        {
            this.entries.push({ kind: "error", text: e.message });
        }

        this.busy = false;
        this.update();
        this.outputEl.scrollTop = this.outputEl.scrollHeight;
    }

    onKeyDown(ev)
    {
        if (ev.key !== "Enter" || this.busy)
            return;
        let cmd = this.cmdInput.value.trim();
        if (!cmd)
            return;
        this.cmdInput.value = "";
        this.runCommand(cmd);
    }

    static template = {
        type: "div",
        class: "console-page",
        $: [
            {
                type: "div",
                class: "console-output",
                bind: "outputEl",
                $: {
                    foreach: { items: c => c.entries, itemKey: (e, ctx) => ctx.index },
                    type: "pre",
                    class: e => `console-entry console-${e.kind}`,
                    text: e => e.kind === "command" ? `> ${e.text}` : e.text,
                }
            },
            {
                type: "div",
                class: "console-input-bar",
                $: {
                    type: "input type=text",
                    class: "console-input",
                    bind: "cmdInput",
                    placeholder: "Enter command…",
                    attr_disabled: c => c.busy || undefined,
                    on_keydown: "onKeyDown",
                }
            }
        ]
    }
}

css`
.console-page
{
    display: flex;
    flex-direction: column;
    height: calc(100vh - var(--header-height));

    .console-output
    {
        flex: 1;
        overflow-y: auto;
        padding: 12px 16px;

        .console-entry
        {
            margin: 0;
            font-family: monospace;
            font-size: 0.85rem;
            white-space: pre-wrap;
            word-break: break-all;
            line-height: 1.5;
        }

        .console-command
        {
            color: var(--accent-color, #4af);
        }

        .console-response
        {
            color: var(--body-fore-color);
        }

        .console-error
        {
            color: var(--danger-color, #c00);
        }
    }

    .console-input-bar
    {
        border-top: 1px solid var(--gridline-color);
        padding: 8px 12px;

        .console-input
        {
            width: 100%;
            font-family: monospace;
            font-size: 0.9rem;
            background: transparent;
            border: none;
            outline: none;
            color: var(--body-fore-color);
            box-sizing: border-box;

            &::placeholder
            {
                opacity: 0.4;
            }

            &:disabled
            {
                opacity: 0.5;
            }
        }
    }
}
`

// Singleton — preserves console history across route navigations
let consolePage = null;

router.register({
    pattern: "/console",
    match: (to) => {
        if (!consolePage)
            consolePage = new ConsolePage();
        to.page = consolePage;
        return true;
    },
});

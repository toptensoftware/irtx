import { Component, css, router, $ } from "@codeonlyjs/core";
import { config } from "./config.js";
import { pack, registerType, registerEnum } from "@toptensoftware/binpack";

// Combined binary file format constants (from @toptensoftware/binpack/cli.js)
const BPAK_SIGNATURE = 0x4B415042;  // "BPAK"
const BPAK_VERSION = 1;
const BPAK_HEADER_SIZE = 32;

function buildCombinedBuffer({ binary, relocations })
{
    const relocTableOffset = BPAK_HEADER_SIZE + binary.length;
    const buf = new Uint8Array(relocTableOffset + relocations.length * 4);
    const view = new DataView(buf.buffer);

    view.setUint32(0, BPAK_SIGNATURE, true);
    view.setUint32(4, BPAK_VERSION, true);
    view.setUint32(8, relocations.length, true);
    view.setUint32(12, relocTableOffset, true);
    // bytes 16-31: zero (no base relocation)

    buf.set(binary, BPAK_HEADER_SIZE);

    // Adjust all pointer fields by HEADER_SIZE and write relocation table
    const delta = BigInt(BPAK_HEADER_SIZE);
    for (let i = 0; i < relocations.length; i++)
    {
        const fileOffset = relocations[i] + BPAK_HEADER_SIZE;
        const ptr = BigInt(view.getUint32(fileOffset, true));
        view.setUint32(fileOffset, Number((ptr + delta) & 0xFFFFFFFFn), true);
        view.setUint32(relocTableOffset + i * 4, fileOffset, true);
    }

    return buf;
}

// Module-level cache: types only need to be registered once per session.
// binpackBlobUrl is kept alive so activities.js can import from it.
let binpackBlobUrl = null;
let registeredRootType = null;

async function ensureBinpackTypes(deviceUrl)
{
    if (binpackBlobUrl)
        return;

    const r = await fetch(`${deviceUrl}/binpack.js`);
    if (!r.ok)
        throw new Error(`Failed to fetch type definitions: HTTP ${r.status}`);

    const source = await r.text();
    const blob = new Blob([source], { type: "application/javascript" });
    const url = URL.createObjectURL(blob);

    const mod = await import(url);
    const typeDefs = mod.default;

    if (!Array.isArray(typeDefs) || typeDefs.length === 0)
        throw new Error("binpack.js must export a non-empty array of type definitions");

    registeredRootType = typeDefs.find(x => x.fields)?.name;
    if (!registeredRootType)
        throw new Error("No root type found in type definitions");

    for (const def of typeDefs)
    {
        try
        {
            if (def.fields)     registerType(def);
            else if (def.enum)  registerEnum(def.name, def.enum);
        }
        catch (e)
        {
            // Ignore "already registered" errors (e.g. page revisited within session)
            if (!e.message?.includes("already registered")) throw e;
        }
    }

    binpackBlobUrl = url;   // keep alive so activities.js blob imports resolve
}

export class ActivitiesEditor extends Component
{
    source = "";
    busy = false;
    result = null;

    constructor()
    {
        super();
        this.doLoad();
    }

    onInput(ev)
    {
        this.source = ev.target.value;
    }

    async doLoad()
    {
        if (this.busy) return;
        this.busy = true;
        this.result = null;
        this.invalidate();

        try
        {
            const r = await fetch(`${config.deviceUrl}/activities.js`);
            if (!r.ok)
                throw new Error(`HTTP ${r.status}`);

            const text = await r.text();
            this.source = text;

            // textArea is bound to the DOM element; set its value directly so
            // the textarea displays the loaded content.
            if (this.textArea)
                this.textArea.value = text;
        }
        catch (e)
        {
            this.result = { ok: false, message: `Load failed: ${e.message}` };
        }
        finally
        {
            this.busy = false;
            this.invalidate();
        }
    }

    async doSave()
    {
        if (this.busy) return;
        this.busy = true;
        this.result = null;
        this.invalidate();

        try
        {
            // Fetch and register type definitions from the device (once per session)
            await ensureBinpackTypes(config.deviceUrl);

            // Rewrite any "binpack:types" imports in activities.js to point at the
            // blob URL we already have for the device's binpack.js module.
            const activitiesSource = this.source.replace(
                /from\s+["']binpack:types["']/g,
                `from "${binpackBlobUrl}"`
            );

            // Dynamically import the activities data from a temporary blob URL
            const activitiesBlob = new Blob([activitiesSource], { type: "application/javascript" });
            const activitiesUrl = URL.createObjectURL(activitiesBlob);
            let data;
            try
            {
                const mod = await import(activitiesUrl);
                data = mod.default;
            }
            finally
            {
                URL.revokeObjectURL(activitiesUrl);
            }

            if (data == null)
                throw new Error("activities.js must export a default value");

            // Pack to binary and wrap in the BPAK combined-file format
            const packResult = pack(registeredRootType, data);
            const binary = buildCombinedBuffer(packResult);

            // Upload helper
            const postFile = async (data, filename, mimeType = "application/octet-stream") => {
                const form = new FormData();
                form.append("file", new Blob([data], { type: mimeType }), filename);
                const r = await fetch(
                    `${config.deviceUrl}/api/upload?filename=${encodeURIComponent(filename)}`,
                    { method: "POST", body: form }
                );
                if (!r.ok)
                    throw new Error(`Upload of ${filename} failed: HTTP ${r.status}`);
            };

            await postFile(new TextEncoder().encode(this.source), "activities.js", "application/javascript");
            await postFile(binary, "activities.bin");

            const r = await fetch(`${config.deviceUrl}/api/reload-activities`, { method: "POST" });
            if (!r.ok)
                throw new Error(`Reload failed: HTTP ${r.status}`);

            this.result = { ok: true, message: "Saved and reloaded successfully." };
        }
        catch (e)
        {
            this.result = { ok: false, message: `Save failed: ${e.message}` };
        }
        finally
        {
            this.busy = false;
            this.invalidate();
        }
    }

    static template = {
        type: "main",
        class: "activities-editor-page",
        $: [
            {
                type: "div .editor-header",
                $: [
                    $.h1("Activities Editor"),
                    {
                        type: "div .toolbar",
                        $: [
                            {
                                type: "button",
                                text: "Reload",
                                on_click: c => c.doLoad(),
                                attr_disabled: c => c.busy || undefined,
                            },
                            {
                                type: "button .save-btn",
                                text: c => c.busy ? "Saving…" : "Save",
                                on_click: c => c.doSave(),
                                attr_disabled: c => c.busy || undefined,
                            },
                        ]
                    },
                ]
            },
            {
                type: "textarea .editor",
                attr_spellcheck: "false",
                bind: "textArea",
                on_input: "onInput",
            },
            {
                if: c => c.result !== null,
                type: "div",
                class: c => `editor-result ${c.result?.ok ? "ok" : "error"}`,
                text: c => c.result?.message,
            },
        ]
    }
}

css`
.activities-editor-page
{
    display: flex;
    flex-direction: column;
    height: calc(100vh - var(--header-height));
    padding: 16px 20px 12px;
    box-sizing: border-box;
    gap: 10px;

    .editor-header
    {
        display: flex;
        align-items: center;
        gap: 16px;
        flex-shrink: 0;

        h1
        {
            font-size: 1.2rem;
            margin: 0;
        }

        .toolbar
        {
            display: flex;
            gap: 8px;

            button
            {
                padding: 4px 16px;
                cursor: pointer;

                &:disabled
                {
                    opacity: 0.5;
                    cursor: default;
                }
            }
        }
    }

    .editor
    {
        flex: 1;
        resize: none;
        font-family: monospace;
        font-size: 0.85rem;
        padding: 8px;
        box-sizing: border-box;
        width: 100%;
        background-color: var(--back-color);
        color: var(--body-fore-color);
        border: 1px solid var(--gridline-color);
        border-radius: 4px;
    }

    .editor-result
    {
        flex-shrink: 0;
        font-size: 0.9rem;

        &.ok    { color: var(--success-color, #4a4); }
        &.error { color: var(--danger-color,  #c00); }
    }
}
`

router.register({
    pattern: "/activities-editor",
    match: (to) => {
        to.page = new ActivitiesEditor();
        return true;
    },
});

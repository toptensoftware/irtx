import { Component, css, router, $ } from "@codeonlyjs/core";
import { config } from "./config.js";
import { pack, registerType, registerEnum } from "@toptensoftware/binpack";
import { basicSetup } from "codemirror";
import { EditorView, keymap, ViewPlugin, Decoration } from "@codemirror/view";
import { EditorState, Compartment, RangeSetBuilder } from "@codemirror/state";
import { indentMore, indentLess } from "@codemirror/commands";
import { javascript } from "@codemirror/lang-javascript";
import { HighlightStyle, syntaxHighlighting, indentUnit } from "@codemirror/language";
import { tags } from "@lezer/highlight";

// ---------------------------------------------------------------------------
// Combined binary file format (mirrors @toptensoftware/binpack/cli.js)
// ---------------------------------------------------------------------------

const BPAK_SIGNATURE  = 0x4B415042;  // "BPAK"
const BPAK_VERSION    = 1;
const BPAK_HEADER_SIZE = 32;

function buildCombinedBuffer({ binary, relocations })
{
    const relocTableOffset = BPAK_HEADER_SIZE + binary.length;
    const buf  = new Uint8Array(relocTableOffset + relocations.length * 4);
    const view = new DataView(buf.buffer);

    view.setUint32(0,  BPAK_SIGNATURE,        true);
    view.setUint32(4,  BPAK_VERSION,           true);
    view.setUint32(8,  relocations.length,     true);
    view.setUint32(12, relocTableOffset,       true);

    buf.set(binary, BPAK_HEADER_SIZE);

    const delta = BigInt(BPAK_HEADER_SIZE);
    for (let i = 0; i < relocations.length; i++)
    {
        const fileOffset = relocations[i] + BPAK_HEADER_SIZE;
        const ptr = BigInt(view.getUint32(fileOffset, true));
        view.setUint32(fileOffset,                  Number((ptr + delta) & 0xFFFFFFFFn), true);
        view.setUint32(relocTableOffset + i * 4,    fileOffset,                          true);
    }

    return buf;
}

// ---------------------------------------------------------------------------
// Binpack type registration cache
// ---------------------------------------------------------------------------

let binpackBlobUrl      = null;
let registeredRootType  = null;

async function ensureBinpackTypes(deviceUrl)
{
    if (binpackBlobUrl)
        return;

    const r = await fetch(`${deviceUrl}/binpack.js`);
    if (!r.ok)
        throw new Error(`Failed to fetch type definitions: HTTP ${r.status}`);

    const source = await r.text();
    const blob   = new Blob([source], { type: "application/javascript" });
    const url    = URL.createObjectURL(blob);
    const mod    = await import(url);
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
            if (!e.message?.includes("already registered")) throw e;
        }
    }

    binpackBlobUrl = url;
}

// ---------------------------------------------------------------------------
// Whitespace visualisation
// ---------------------------------------------------------------------------

const spaceDeco = Decoration.mark({ class: "cm-vis-space" });
const tabDeco   = Decoration.mark({ class: "cm-vis-tab" });

function buildWhitespaceDeco(view)
{
    const builder = new RangeSetBuilder();
    for (const { from, to } of view.visibleRanges)
    {
        const text = view.state.doc.sliceString(from, to);
        for (let i = 0; i < text.length; i++)
        {
            if (text[i] === " ")       builder.add(from + i, from + i + 1, spaceDeco);
            else if (text[i] === "\t") builder.add(from + i, from + i + 1, tabDeco);
        }
    }
    return builder.finish();
}

const showWhitespace = ViewPlugin.fromClass(
    class {
        constructor(view)  { this.decorations = buildWhitespaceDeco(view); }
        update(update)     {
            if (update.docChanged || update.viewportChanged)
                this.decorations = buildWhitespaceDeco(update.view);
        }
    },
    { decorations: v => v.decorations }
);

// ---------------------------------------------------------------------------
// CodeMirror theme helpers
// ---------------------------------------------------------------------------

// Syntax highlight styles.  Colors chosen to match VS Code Dark+ / Light+.
const darkHighlight = HighlightStyle.define([
    { tag: tags.keyword,                            color: "#569cd6" },
    { tag: tags.string,                             color: "#ce9178" },
    { tag: tags.comment,                            color: "#6a9955", fontStyle: "italic" },
    { tag: tags.number,                             color: "#b5cea8" },
    { tag: [tags.bool, tags.null],                  color: "#569cd6" },
    { tag: tags.propertyName,                       color: "#9cdcfe" },
    { tag: tags.function(tags.variableName),        color: "#dcdcaa" },
    { tag: tags.definition(tags.variableName),      color: "#4fc1ff" },
    { tag: tags.className,                          color: "#4ec9b0" },
    { tag: tags.operator,                           color: "#d4d4d4" },
    { tag: tags.punctuation,                        color: "#d4d4d4" },
]);

const lightHighlight = HighlightStyle.define([
    { tag: tags.keyword,                            color: "#0000ff" },
    { tag: tags.string,                             color: "#a31515" },
    { tag: tags.comment,                            color: "#008000", fontStyle: "italic" },
    { tag: tags.number,                             color: "#098658" },
    { tag: [tags.bool, tags.null],                  color: "#0000ff" },
    { tag: tags.propertyName,                       color: "#001080" },
    { tag: tags.function(tags.variableName),        color: "#795e26" },
    { tag: tags.definition(tags.variableName),      color: "#001080" },
    { tag: tags.className,                          color: "#267f99" },
]);

// Build the set of CM6 extensions that control theme + highlight for the
// current dark-mode state.  Reading CSS vars at call time means we always
// pick up the correct resolved values.
function makeThemeExtensions(isDark)
{
    const style    = getComputedStyle(document.documentElement);
    const bodyBg   = style.getPropertyValue("--body-back-color").trim();
    const bodyFg   = style.getPropertyValue("--body-fore-color").trim();
    const inputBg  = style.getPropertyValue("--input-back-color").trim();
    const gridline = style.getPropertyValue("--gridline-color").trim();
    const accent   = style.getPropertyValue("--accent-color").trim();

    return [
        EditorView.darkTheme.of(isDark),
        EditorView.theme({
            "&":                            { backgroundColor: inputBg, color: bodyFg, height: "100%" },
            "&.cm-focused":                 { outline: "none" },
            ".cm-scroller":                 { fontFamily: "monospace", fontSize: "0.85rem", lineHeight: "1.6" },
            ".cm-content":                  { caretColor: bodyFg },
            ".cm-cursor":                   { borderLeftColor: bodyFg },
            ".cm-selectionBackground, &.cm-focused .cm-selectionBackground":
                                            { backgroundColor: `${accent}40` },
            ".cm-gutters":                  { backgroundColor: bodyBg, color: `${bodyFg}80`,
                                              border: "none", borderRight: `1px solid ${gridline}` },
            ".cm-activeLineGutter":         { backgroundColor: `${bodyFg}12` },
            ".cm-activeLine":               { backgroundColor: `${bodyFg}08` },
            ".cm-matchingBracket":          { outline: `1px solid ${accent}80`, borderRadius: "2px" },

            // Whitespace visualisation — dots for spaces, tick for tabs
            ".cm-vis-space":  {
                backgroundImage: `radial-gradient(circle at center, ${bodyFg}50 1.2px, transparent 1.2px)`,
                backgroundRepeat: "no-repeat",
                backgroundPosition: "center 60%",
                backgroundSize: "100% 100%",
            },
            ".cm-vis-tab":    { position: "relative" },
        }, { dark: isDark }),
        // syntaxHighlighting without { fallback:true } takes precedence over
        // basicSetup's syntaxHighlighting(defaultHighlightStyle, { fallback:true })
        syntaxHighlighting(isDark ? darkHighlight : lightHighlight),
    ];
}

// Tab: with a selection indent the selected lines; otherwise insert enough
// spaces to advance the cursor to the next 4-column tab stop.
function smartTab({ state, dispatch })
{
    if (!state.selection.main.empty)
        return indentMore({ state, dispatch });

    const { from } = state.selection.main;
    const col    = from - state.doc.lineAt(from).from;
    const spaces = " ".repeat(4 - (col % 4));
    dispatch(state.update(state.replaceSelection(spaces), {
        scrollIntoView: true,
        userEvent: "input",
    }));
    return true;
}


// ---------------------------------------------------------------------------
// ActivitiesEditor component
// ---------------------------------------------------------------------------

export class ActivitiesEditor extends Component
{
    source      = "";
    busy        = false;
    result      = null;

    #editorView         = null;
    #themeCompartment   = new Compartment();
    #onThemeChange      = null;

    constructor()
    {
        super();

        // Clean up the CM6 editor and theme listener when navigating away
        const onDidEnter = (from, to) => {
            if (to.page !== this)
            {
                this.#destroyEditor();
                router.removeEventListener("didEnter", onDidEnter);
            }
        };
        router.addEventListener("didEnter", onDidEnter);

        this.doLoad();
    }

    // CodeOnly calls `this.editorContainer = el` when the bind resolves.
    // We use a setter so we can mount CM6 as soon as the element is ready.
    #editorContainer = null;
    get editorContainer() { return this.#editorContainer; }
    set editorContainer(el)
    {
        this.#editorContainer = el;
        if (el && !this.#editorView)
            this.#mountEditor(el);
    }

    #mountEditor(el)
    {
        const isDark = window.stylish?.darkMode ?? false;

        this.#editorView = new EditorView({
            state: EditorState.create({
                doc: this.source,
                extensions: [
                    basicSetup,
                    indentUnit.of("    "),
                    keymap.of([{ key: "Tab", run: smartTab }, { key: "Shift-Tab", run: indentLess }]),
                    showWhitespace,
                    javascript(),
                    this.#themeCompartment.of(makeThemeExtensions(isDark)),
                    EditorView.updateListener.of(update => {
                        if (update.docChanged)
                            this.source = update.state.doc.toString();
                    }),
                ],
            }),
            parent: el,
        });

        this.#onThemeChange = (e) => {
            this.#editorView?.dispatch({
                effects: this.#themeCompartment.reconfigure(makeThemeExtensions(e.darkMode)),
            });
        };
        window.stylish?.addEventListener("darkModeChanged", this.#onThemeChange);
    }

    #destroyEditor()
    {
        if (this.#onThemeChange)
        {
            window.stylish?.removeEventListener("darkModeChanged", this.#onThemeChange);
            this.#onThemeChange = null;
        }
        this.#editorView?.destroy();
        this.#editorView = null;
    }

    async doLoad()
    {
        if (this.busy) return;
        this.busy   = true;
        this.result = null;
        this.invalidate();

        try
        {
            const r = await fetch(`${config.deviceUrl}/activities.js`);
            if (!r.ok)
                throw new Error(`HTTP ${r.status}`);

            const text = await r.text();
            this.source = text;

            // If the editor is already mounted, update its document
            if (this.#editorView)
            {
                const state = this.#editorView.state;
                this.#editorView.dispatch({
                    changes: { from: 0, to: state.doc.length, insert: text },
                });
            }
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
        this.busy   = true;
        this.result = null;
        this.invalidate();

        try
        {
            await ensureBinpackTypes(config.deviceUrl);

            // Read current text from editor (or source property if editor isn't up yet)
            const currentSource = this.#editorView
                ? this.#editorView.state.doc.toString()
                : this.source;

            const activitiesSource = currentSource.replace(
                /from\s+["']binpack:types["']/g,
                `from "${binpackBlobUrl}"`
            );

            const activitiesBlob = new Blob([activitiesSource], { type: "application/javascript" });
            const activitiesUrl  = URL.createObjectURL(activitiesBlob);
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

            const packResult = pack(registeredRootType, data);
            const binary     = buildCombinedBuffer(packResult);

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

            await postFile(new TextEncoder().encode(currentSource), "activities.js", "application/javascript");
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
            this.busy   = false;
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
                type: "div .editor-container",
                bind: "editorContainer",
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

    .editor-container
    {
        flex: 1;
        overflow: hidden;
        border: 1px solid var(--gridline-color);
        border-radius: 4px;

        /* CM6 mounts a .cm-editor div inside; make it fill the container */
        .cm-editor
        {
            height: 100%;
        }

        .cm-scroller
        {
            overflow: auto;
        }

        .cm-vis-tab::before
        {
            content: '→';
            position: absolute;
            left: 1px;
            opacity: 0.35;
            line-height: inherit;
            pointer-events: none;
        }
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

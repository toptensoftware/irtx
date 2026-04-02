import { Component, css, router, $ } from "@codeonlyjs/core";
import { config } from "./config.js";

export class ActivitiesPage extends Component
{
    busy = false;
    result = null;   // { ok: bool, message: string }

    get selectedFile()
    {
        return this.fileInput?.files[0] ?? null;
    }

    get selectedSourceFile()
    {
        return this.sourceFileInput?.files[0] ?? null;
    }

    async upload()
    {
        if (this.busy || !this.selectedFile)
            return;

        this.busy = true;
        this.result = null;
        this.invalidate();

        try
        {
            const postFile = async (file, filename) => {
                let body = new FormData();
                body.append("file", file);
                return fetch(`${config.deviceUrl}/api/upload?filename=${encodeURIComponent(filename)}`, {
                    method: "POST",
                    body,
                });
            };

            let r = await postFile(this.selectedFile, "activities.bin");
            if (!r.ok)
            {
                this.result = { ok: false, message: `Binary upload failed: HTTP ${r.status}` };
                return;
            }

            if (this.selectedSourceFile)
            {
                let r2 = await postFile(this.selectedSourceFile, "activities.js");
                if (!r2.ok)
                {
                    this.result = { ok: false, message: `Source upload failed: HTTP ${r2.status}` };
                    return;
                }
            }

            let r3 = await fetch(`${config.deviceUrl}/api/reload-activities`, { method: "POST" });
            if (!r3.ok)
            {
                this.result = { ok: false, message: `Reload failed: HTTP ${r3.status}` };
                return;
            }

            this.result = { ok: true, message: "Upload successful." };
        }
        catch (e)
        {
            this.result = { ok: false, message: `Upload failed: ${e.message}` };
        }
        finally
        {
            this.busy = false;
            this.invalidate();
        }
    }

    onSubmit(ev)
    {
        ev.preventDefault();
        this.upload();
    }

    static template = {
        type: "main",
        class: "activities-page",
        $: [
            $.h1("Upload Activities"),
            $.p("Select a compiled activities.bin file to upload to the device. Optionally also upload the source activities.js to keep a reference copy on the device."),
            {
                type: "form",
                on_submit: "onSubmit",
                $: [
                    {
                        type: "div",
                        class: "field",
                        $: [
                            $.label.attr_for("file-input").text("Binary (activities.bin)"),
                            {
                                type: "input type=file",
                                id: "file-input",
                                attr_accept: ".bin",
                                bind: "fileInput",
                            },
                        ]
                    },
                    {
                        type: "div",
                        class: "field",
                        $: [
                            $.label.attr_for("source-file-input").text("Source (activities.js, optional)"),
                            {
                                type: "input type=file",
                                id: "source-file-input",
                                attr_accept: ".js",
                                bind: "sourceFileInput",
                            },
                        ]
                    },
                    {
                        type: "button type=submit",
                        class: "upload-btn",
                        attr_disabled: c => c.busy || undefined,
                        text: c => c.busy ? "Uploading…" : "Upload",
                    },
                ]
            },
            {
                if: c => c.result !== null,
                type: "div",
                class: c => `upload-result ${c.result?.ok ? "ok" : "error"}`,
                text: c => c.result?.message,
            },
        ]
    }
}

css`
.activities-page
{
    padding: 20px 30px;
    max-width: 600px;

    h1
    {
        font-size: 1.2rem;
        margin-bottom: 0.5rem;
    }

    p
    {
        opacity: 0.7;
        font-size: 0.9rem;
        margin-bottom: 1.5rem;
    }

    .field
    {
        display: flex;
        flex-direction: column;
        gap: 6px;
        margin-bottom: 1rem;

        label
        {
            font-size: 0.8rem;
            text-transform: uppercase;
            letter-spacing: 0.08em;
            opacity: 0.6;
        }
    }

    .upload-btn
    {
        padding: 6px 20px;
        cursor: pointer;

        &:disabled
        {
            opacity: 0.5;
            cursor: default;
        }
    }

    .upload-result
    {
        margin-top: 1rem;
        font-size: 0.9rem;

        &.ok    { color: var(--success-color, #4a4); }
        &.error { color: var(--danger-color,  #c00); }
    }
}
`

router.register({
    pattern: "/activities",
    match: (to) => {
        to.page = new ActivitiesPage();
        return true;
    },
});

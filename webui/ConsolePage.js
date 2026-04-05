import { Component, css, router } from "@codeonlyjs/core";
import { config } from "./config.js";
import { Terminal } from "xterm";
import { FitAddon } from "@xterm/addon-fit";
import "xterm/css/xterm.css";

class ConsolePage extends Component
{
    // Lazily initialised — Terminal.open() requires the element to be in the DOM,
    // so we initialise on first mount rather than in the constructor.
    #terminal        = null;
    #fitAddon        = null;
    #resizeObserver  = null;
    #termContainer   = null;   // manually created — not owned by CodeOnly template
    #ws              = null;
    #mounted         = false;
    #initialized     = false;

    get #wsUrl()
    {
        // In dev mode config.deviceUrl is the device origin (e.g. "http://10.1.1.101").
        // In prod mode it is empty and we connect to the same host.
        const base = config.deviceUrl || window.location.origin;
        const url  = new URL(base);
        return `ws://${url.hostname}:81`;
    }

    #makeTheme(isDark)
    {
        if (isDark)
        {
            return {
                background:                    "#1e1e1e",
                foreground:                    "#dedede",
                cursor:                        "#dedede",
                cursorAccent:                  "#1e1e1e",
                selectionBackground:           "rgba(255, 255, 255, 0.25)",
                selectionForeground:           "#ffffff",
                selectionInactiveBackground:   "rgba(255, 255, 255, 0.15)",
            };
        }
        else
        {
            return {
                background:                    "#ffffff",
                foreground:                    "#000000",
                cursor:                        "#000000",
                cursorAccent:                  "#ffffff",
                selectionBackground:           "rgba(0, 0, 0, 0.2)",
                selectionForeground:           "#000000",
                selectionInactiveBackground:   "rgba(0, 0, 0, 0.1)",
            };
        }
    }

    #onThemeChange = (e) =>
    {
        if (this.#terminal)
            this.#terminal.options.theme = this.#makeTheme(e.darkMode);
    };

    onMount()
    {
        this.#mounted = true;

        if (!this.#initialized)
        {
            this.#initialized = true;

            // Create the terminal container ourselves so CodeOnly never re-renders it
            this.#termContainer = document.createElement("div");
            this.#termContainer.className = "console-terminal";
        }

        // Re-append every mount — CodeOnly may recreate termEl on each render cycle
        if (!this.termEl.contains(this.#termContainer))
        {
            this.termEl.appendChild(this.#termContainer);
        }

        window.stylish?.addEventListener("darkModeChanged", this.#onThemeChange);

        if (!this.#terminal)
        {

            this.#terminal = new Terminal({
                convertEol:  true,    // translate \n → \r\n client-side
                cursorBlink: true,
                fontSize:    14,
                fontFamily:  "monospace",
                theme:       this.#makeTheme(window.stylish?.darkMode ?? true),
            });

            this.#fitAddon = new FitAddon();
            this.#terminal.loadAddon(this.#fitAddon);
            this.#terminal.open(this.#termContainer);

            // Forward keystrokes to the device
            this.#terminal.onData(data => {
                if (this.#ws?.readyState === WebSocket.OPEN)
                    this.#ws.send(data);
            });

            this.#resizeObserver = new ResizeObserver(() => this.#fitAddon?.fit());
            this.#resizeObserver.observe(this.#termContainer);
        }
        else
        {
            // Re-apply current theme in case it changed while the page was unmounted
            this.#terminal.options.theme = this.#makeTheme(window.stylish?.darkMode ?? true);
        }

        requestAnimationFrame(() => {
            if (!this.#mounted) return;
            this.#fitAddon?.fit();
            this.#terminal?.refresh(0, this.#terminal.rows - 1);
            this.#terminal?.focus();
        });
        if (!this.#ws || this.#ws.readyState === WebSocket.CLOSED)
            this.#connect();
    }

    onUnmount()
    {
        this.#mounted = false;
        window.stylish?.removeEventListener("darkModeChanged", this.#onThemeChange);
        // Keep the WebSocket alive — page is a singleton, reconnect on next mount if needed
    }

    #connect()
    {
        if (!this.#mounted || !this.#terminal) return;

        this.#terminal.writeln("\x1b[33mConnecting…\x1b[0m");

        const ws   = new WebSocket(this.#wsUrl);
        this.#ws   = ws;

        ws.onopen = () => {
            if (this.#ws !== ws) return;
            this.#terminal?.writeln("\x1b[32mConnected\x1b[0m");
        };

        ws.onmessage = (e) => {
            if (this.#ws !== ws) return;
            this.#terminal?.write(e.data);
        };

        ws.onclose = () => {
            if (this.#ws !== ws) return;
            this.#ws = null;
            this.#terminal?.writeln("\x1b[31m\r\n[disconnected — retrying in 3 s]\x1b[0m");
            setTimeout(() => this.#connect(), 3000);
        };
    }

    static template = {
        type: "div",
        class: "console-page",
        bind: "termEl",
    }
}

css`
.console-page
{
    display: flex;
    flex-direction: column;
    height: calc(100vh - var(--header-height));
    padding: 8px;
    box-sizing: border-box;
    background: var(--body-back-color);
}

.console-terminal
{
    flex: 1;
    min-height: 0;
    position: relative;   /* required so xterm's absolute children size correctly */
}
`

// Singleton — keeps the terminal alive across route navigations
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

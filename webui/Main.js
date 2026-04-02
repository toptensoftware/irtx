import { Component, css, router } from "@codeonlyjs/core";
import { Header } from "./Header.js";
import { Meta } from "./Meta.js";

import "./HomePage.js";
import "./NotFoundPage.js";

// Main application
class Main extends Component
{
    constructor()
    {
        super();
        this.page = null;

        router.addEventListener("didEnter", (from, to) => {

            // Load navigated page into router slot
            if (to.page)
            {
                this.page = to.page;
                this.invalidate();
            }

        });
    }

    static template = {
        type: "div",
        $: [
            Header,
            {
                type: "div #layoutRoot",
                $: {
                    type: "embed-slot",
                    content: c => c.page,
                }
            }
        ]
    }
}

css`
#layoutRoot
{
    padding-top: var(--header-height);
}

`;

// Main entry point, create Application and mount
export function main()
{
    new Meta().mount("head");
    new Main().mount("body");
    router.start();
}
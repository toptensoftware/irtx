import { Component, router } from "@codeonlyjs/core";

export class NotFoundPage extends Component
{
    constructor(url)
    {
        super();
        this.url = url;
    }

    static template = {
        type: "div",
        class: "center",
        $: [
            {
                type: "h1",
                class: "danger",
                text: "Page not found! 😟",
            },
            {
                type: "p",
                text: c => `The page ${c.url} doesn't exist!`
            },
            {
                type: "p",
                $: {
                    type: "a",
                    href: "/",
                    text: "Return Home",
                }
            }
        ]
    };
}


router.register({
    match: (to) => {
        to.page = new NotFoundPage(to.url);
        return true;
    },
    order: 1000,
});
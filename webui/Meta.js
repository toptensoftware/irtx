import { Component, router, $ } from "@codeonlyjs/core";
import { config } from "./config.js"

export class Meta extends Component
{
    constructor()
    {
        super();

        router.addEventListener("didEnter", (from, to) => {
            this.invalidate();
            if (coenv.browser)
                document.title = this.title;
        });
    }

    get title()
    {
        if (router.current?.title)
            return `${router.current.title} - ${config.appName}`;
        else
            return config.appName;
    }

    get name()
    {
        return config.appName;
    }

    get description()
    {
        return config.description;
    }

    get url()
    {
        return router.current?.url.href ?? "";
    }

    get image()
    {
        return null;
    }

    static template = [

        // Standard
        $.title(c => c.title),
        $.meta.name("description").content(c => c.description),

        // Google/Search
        $.meta.itemprop("name").content(c => c.name),
        $.meta.itemprop("description").content(c => c.description),
        $.meta.itemprop("image").content(c => c.image).if(c => c.image),

        // Facebook
        $.meta.name("og:url").content(c => c.url),
        $.meta.name("og:type").content("website"),
        $.meta.name("og:title").content(c => c.title),
        $.meta.name("og:description").content(c => c.description),
        $.meta.name("og:image").content(c => c.image).if(c => c.image),

        // Twitter
        $.meta.name("twitter:card").content("summary_large_image"),
        $.meta.name("twitter:title").content(c => c.title),
        $.meta.name("twitter:description").content(c => c.description),
        $.meta.name("twitter:image").content(c => c.image).if(c => c.image),

        // Apple
        $.link.rel("apple-touch-icon").href(c => c.image).if(c => c.image),
        
    ]
}

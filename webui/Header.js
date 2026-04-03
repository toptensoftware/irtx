import { Component, css, html } from "@codeonlyjs/core";
import { config } from "./config.js";

// The main header
export class Header extends Component
{
    static template = {
        type: "header #header",
        $: [
            {
                type: "a .title",
                href: "/",
                $: [
                    { 
                        type: "img", 
                        src: "/public/logo.svg",
                    },
                    config.appName,
                ]
            },
            {
                type: "nav .nav-links",
                $: [
                    { type: "a", attr_href: "/", text: "Status" },
                    { type: "a", attr_href: "/activities-editor", text: "Activities" },
                    //{ type: "a", attr_href: "/activities", text: "Activities" },
                    { type: "a", attr_href: "/console", text: "Console" },
                    { type: "a", attr_href: "/dmesg", text: "Log" },
                ]
            },
            {
                type: "div .buttons",
                $: [
                    {
                        type: "input type=checkbox .theme-switch",
                        on_click: () => window.stylish.toggleTheme(),
                    },
                    {
                        // Initialize the state of the theme-switch.
                        // We do this as early as possible to prevent it flicking on/off as page hydrates.
                        type: "script",
                        text: html(`document.querySelector(".theme-switch").checked = window.stylish.darkMode;`),
                    }                    
                ]
            }
        ]
    }
}

css`
:root
{
    --header-height: 50px;
}

#header
{
    position: fixed;
    top: 0;
    width: 100%;
    height: var(--header-height);

    display: flex;
    justify-content: start;
    align-items: center;
    border-bottom: 1px solid var(--gridline-color);
    padding-left: 10px;
    padding-right: 10px;
    background-color: rgb(from var(--back-color) r g b / 75%);
    z-index: 1;

    .title 
    {
        flex-grow: 1;
        display: flex;
        align-items: center;
        color: var(--body-fore-color);
        transition: opacity 0.2s;

        &:hover
        {
            opacity: 75%;
        }

        img
        {
            height: calc(var(--header-height) - 25px);
            padding-right: 10px
        }
    }


    .nav-links
    {
        display: flex;
        gap: 20px;
        margin-right: 20px;

        a
        {
            color: var(--body-fore-color);
            text-decoration: none;
            font-size: 0.9rem;
            opacity: 0.7;
            transition: opacity 0.15s;

            &:hover
            {
                opacity: 1;
            }
        }
    }

    .buttons
    {
        font-size: 12pt;
        display: flex;
        gap: 10px;
        align-items: center;

        .theme-switch
        {
            transform: translateY(-1.5px);
        }
    }
}
`
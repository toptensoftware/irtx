const config = {
    development: {
        bundleFree: {
            modules: [ 
                "@codeonlyjs/core",
            ],
            replace: [
                { from: "./Main.js", to: "/Main.js" },
            ],
        },
    }
};

export default config;
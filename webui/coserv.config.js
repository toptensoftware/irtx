const config = {
    development: {
        bundleFree: {
            modules: [ 
                "@codeonlyjs/core",
            ],
            replace: [
                { from: "./Main.js", to: "/Main.js" },
                { from: "main();", to: "main(\"http://10.1.1.101\");" }
            ],
        },
    }
};

export default config;
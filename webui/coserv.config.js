const config = {
    development: {
        bundleFree: {
            modules: [ 
                "@codeonlyjs/core",
            ],
            replace: [
                { from: "./Main.js", to: "/Main.js" },
                //{ from: "deviceUrl: \"\"", to: "deviceUrl: \"http://192.168.4.1\"" }
            ],
        },
    }
};

export default config;
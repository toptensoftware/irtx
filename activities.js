const activitiesRoot = {
    version: 1,
    devices: [
        { 
            name: "tv", 
            onCommands: [], 
            offCommands: [] 
        },
        { 
            name: "receiver", 
            onCommands: [], 
            offCommands: [] 
        },
        { 
            name: "dvr", 
            onCommands: [], 
            offCommands: [] 
        },
        { 
            name: "atv", 
            onCommands: [], 
            offCommands: [] 
        },
    ],
    activities: [
        {
            name: "off",
            deviceStates: [
                { name: "atv", on: false },
                { name: "dvr", on: false },
            ],
        }
    ],
};

export default activitiesRoot;

#!/usr/bin/env node

/**
 * generate_compile_commands.js
 * 
 * Usage:
 *   arduino-cli compile --fqbn esp32:esp32:esp32 --verbose . | node generate_compile_commands.js
 */

const fs = require("fs");

// Load previous compile_commands.json if it exists
const JSON_PATH = "./compile_commands.json";
let oldEntries = {};
if (fs.existsSync(JSON_PATH)) {
  try {
    const oldJson = JSON.parse(fs.readFileSync(JSON_PATH, "utf-8"));
    for (const entry of oldJson) {
      oldEntries[entry.file] = entry;
    }
  } catch (err) {
    console.warn("Warning: Could not parse old compile_commands.json, ignoring previous entries.");
  }
}

// Read stdin
let input = "";
process.stdin.setEncoding("utf8");
process.stdin.on("data", chunk => input += chunk);
process.stdin.on("end", () => {
  const lines = input.split(/\r?\n/);
  const entries = {};
  let compilingSketch = false;

  for (let line of lines) {
    line = line.trim();

    // Detect sketch compilation start
    if (line.startsWith("Compiling sketch")) {
      compilingSketch = true;
      continue;
    }

    // Detect library compilation start
    if (line.startsWith("Compiling libraries") || line.startsWith("Using library")) {
      compilingSketch = false;
      continue;
    }

    // Handle skipped files
    if (line.startsWith("Using previously compiled file:")) {
      const match = line.match(/Using previously compiled file:\s*(\S+)/);
      if (match) {
        const filePath = match[1].replace(/\\/g, "/");
        if (oldEntries[filePath]) {
          entries[filePath] = oldEntries[filePath];
        }
      }
      continue;
    }

    // Capture compiler command
    if (compilingSketch && (line.includes("g++") || line.includes("gcc"))) {
      const match = line.match(/^(.*g\+\+.*?)-o\s+\S+\s+(\S+)$/);
      let inputFile = null;
      let cmd = line;

      if (match) {
        inputFile = match[2].replace(/\\/g, "/");
      } else {
        // fallback: pick last token as input file
        const parts = line.split(/\s+/);
        inputFile = parts[parts.length - 1].replace(/\\/g, "/");
      }

      // Add entry
      entries[inputFile] = {
        directory: process.cwd().replace(/\\/g, "/"),
        command: cmd,
        file: inputFile
      };
    }
  }

  // Write compile_commands.json
  const jsonArray = Object.values(entries);
  fs.writeFileSync(JSON_PATH, JSON.stringify(jsonArray, null, 2), "utf-8");
  console.log(`Generated ${JSON_PATH} with ${jsonArray.length} entries.`);
});
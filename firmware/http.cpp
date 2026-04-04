#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "config.h"
#include "activities.h"
#include "http.h"
#include "web_files.h"
#include "commands.h"
#include "console.h"
#include "log.h"

static WebServer server(80);

// ---- Handlers ----

static void addCorsHeaders()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
}

static void handleNotFound()
{
    server.send(404, "text/plain", "Not found");
}

static const char* mimeTypeForPath(const String& path)
{
    struct { const char* ext; const char* mime; } table[] = {
        { ".html", "text/html" },
        { ".css",  "text/css" },
        { ".js",   "application/javascript" },
        { ".json", "application/json" },
        { ".txt",  "text/plain" },
        { ".xml",  "application/xml" },
        { ".bin",  "application/octet-stream" },
        { ".png",  "image/png" },
        { ".jpg",  "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".gif",  "image/gif" },
        { ".svg",  "image/svg+xml" },
        { ".ico",  "image/x-icon" },
        { ".pdf",  "application/pdf" },
        { ".gz",   "application/gzip" },
        { ".zip",  "application/zip" },
    };
    for (auto& e : table)
    {
        if (path.endsWith(e.ext))
            return e.mime;
    }
    return "application/octet-stream";
}

static void handleWebFile()
{
    String path = server.uri();
    if (path == "/")
        path = "/index.html";

    // 1. Built-in web UI files (gzip-compressed, served from flash)
    for (int i = 0; i < web_files_count; i++)
    {
        if (path == web_files[i].path)
        {
            addCorsHeaders();
            server.sendHeader("Content-Encoding", "gzip");
            server.send_P(200, web_files[i].content_type,
                          (const char*)web_files[i].data,
                          web_files[i].size);
            return;
        }
    }

    // 2. User files on LittleFS
    if (LittleFS.exists(path))
    {
        File f = LittleFS.open(path, "r");
        if (f)
        {
            addCorsHeaders();
            server.streamFile(f, mimeTypeForPath(path));
            f.close();
            return;
        }
    }

    // 3. SPA fallback: for GET/HEAD return index.html so the client-side router
    // can handle the URL. All other methods get a 404.
    if (server.method() == HTTP_GET || server.method() == HTTP_HEAD)
    {
        for (int i = 0; i < web_files_count; i++)
        {
            if (strcmp(web_files[i].path, "/index.html") == 0)
            {
                server.sendHeader("Content-Encoding", "gzip");
                server.send_P(200, web_files[i].content_type,
                              (const char*)web_files[i].data,
                              web_files[i].size);
                return;
            }
        }
    }

    handleNotFound();
}

static File uploadFile;
static int uploadBytesWritten;
static char uploadFilename[64];

static void handleFileUpload()
{
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = server.arg("filename");
        if (filename.length() == 0)
        {
            LOG("HTTP: /api/upload missing filename param\n");
            uploadFilename[0] = '\0';
            return;
        }
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        filename.toCharArray(uploadFilename, sizeof(uploadFilename));
        uploadBytesWritten = 0;
        uploadFile = LittleFS.open(uploadFilename, "w");
        if (!uploadFile)
            LOG("HTTP: failed to open %s for writing\n", uploadFilename);
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile)
            uploadBytesWritten += uploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile)
        {
            uploadFile.close();
            LOG("HTTP: wrote %d bytes to %s\n", uploadBytesWritten, uploadFilename);
        }
    }
}

static void handlePostUpload()
{
    addCorsHeaders();
    if (uploadFilename[0] == '\0')
        server.send(400, "text/plain", "missing filename param\n");
    else
        server.send(200, "text/plain", "OK");
}

static void handlePostReloadActivities()
{
    addCorsHeaders();
    reloadActivities();
    server.send(200, "text/plain", "OK");
}

static void handleGetStatus()
{
    addCorsHeaders();
    CaptureConsole cap;
    consoleSetActive(&cap);
    handleCommand("status");
    consoleSetActive(nullptr);
    server.send(200, "application/json", cap.output);
}

static void handleGetDmesg()
{
    addCorsHeaders();
    CaptureConsole cap;
    consoleSetActive(&cap);
    dmesgPrint();
    consoleSetActive(nullptr);
    server.send(200, "text/plain", cap.output);
}

static void handlePostCommand()
{
    addCorsHeaders();
    String cmd = server.arg("plain");
    cmd.trim();
    if (cmd.length() == 0)
    {
        server.send(400, "text/plain", "No command\n");
        return;
    }
    CaptureConsole cap;
    consoleSetActive(&cap);
    handleCommand(cmd.c_str());
    consoleSetActive(nullptr);
    server.send(200, "text/plain", cap.output);
}

// ---- Setup ----

void setupHttp()
{
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP)
        return;

    const char *collectHeaderKeys[] = { "Content-Length" };
    server.collectHeaders(collectHeaderKeys, 1);
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/api/dmesg", HTTP_GET, handleGetDmesg);
    server.on("/api/command", HTTP_POST, handlePostCommand);
    server.on("/api/upload", HTTP_POST, handlePostUpload, handleFileUpload);
    server.on("/api/reload-activities", HTTP_POST, handlePostReloadActivities);
    server.onNotFound(handleWebFile);

    server.begin();
    LOG("HTTP server listening on port 80\n");
}

// ---- Poll (called from loop) ----

void pollHttp()
{
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP)
        return;

    server.handleClient();
}

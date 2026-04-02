#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "config.h"
#include "activities.h"
#include "http.h"
#include "web_files.h"
#include "serial.h"
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

static void handleWebFile()
{
    String path = server.uri();
    if (path == "/")
        path = "/index.html";

    for (int i = 0; i < web_files_count; i++)
    {
        if (path == web_files[i].path)
        {
            server.sendHeader("Content-Encoding", "gzip");
            server.send_P(200, web_files[i].content_type,
                          (const char*)web_files[i].data,
                          web_files[i].size);
            return;
        }
    }

    // SPA fallback: for GET/HEAD return index.html so the client-side router
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

static void handleActivitiesUpload()
{
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        uploadBytesWritten = 0;
        uploadFile = LittleFS.open("/activities.bin", "w");
        if (!uploadFile)
            LOG("HTTP: failed to open /activities.bin for writing\n");
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
            LOG("HTTP: wrote %d bytes to /activities.bin\n", uploadBytesWritten);
            reloadActivities();
        }
    }
}

static void handleGetStatus()
{
    addCorsHeaders();
    String output;
    logStartCapture(&output);
    handleCommand("status");
    logEndCapture();
    server.send(200, "application/json", output);
}

static void handleGetDmesg()
{
    addCorsHeaders();
    String output;
    logStartCapture(&output);
    dmesgPrint();
    logEndCapture();
    server.send(200, "text/plain", output);
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
    String output;
    logStartCapture(&output);
    handleCommand(cmd.c_str());
    logEndCapture();
    server.send(200, "text/plain", output);
}

static void handlePostActivities()
{
    addCorsHeaders();
    server.send(200, "text/plain", "OK");
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
    server.on("/api/activities", HTTP_POST, handlePostActivities, handleActivitiesUpload);
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

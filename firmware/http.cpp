#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "config.h"
#include "activities.h"
#include "http.h"

static WebServer server(80);

// ---- Handlers ----

static void handleNotFound()
{
    server.send(404, "text/plain", "Not found");
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

static void handlePostActivities()
{
    server.send(200, "text/plain", "OK");
}

// ---- Setup ----

void setupHttp()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    const char *collectHeaderKeys[] = { "Content-Length" };
    server.collectHeaders(collectHeaderKeys, 1);
    server.on("/activities", HTTP_POST, handlePostActivities, handleActivitiesUpload);
    server.onNotFound(handleNotFound);

    server.begin();
    LOG("HTTP server listening on port 80\n");
}

// ---- Poll (called from loop) ----

void pollHttp()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    server.handleClient();
}

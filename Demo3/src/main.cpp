/**
  @file
  @author Alexander Nikolaev
*/

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <httpserver/httplistener.h>
#include <logging/filelogger.h>
#include <httpserver/httpheadershandler.h>
#include "requesthandler.h"

using namespace stefanfrings;

/**
  Entry point of the program.
*/
int main(int argc, char *argv[])
{
    // Initialize the core application
    QCoreApplication app(argc, argv);
    app.setApplicationName("Demo3");

    // Collect hardcoded configarion settings
    QSettings* settings=new QSettings("settings", QSettings::IniFormat, &app);
    // settings->setValue("host","192.168.0.100");
    settings->setValue("port","8080");
    settings->setValue("minThreads","4");
    settings->setValue("maxThreads","100");
    settings->setValue("cleanupInterval","60000");
    settings->setValue("readTimeout","60000");
    settings->setValue("maxRequestSize","16000");
    settings->setValue("maxMultiPartSize","10000000");
    // settings->setValue("sslKeyFile","ssl/my.key");
    // settings->setValue("sslCertFile","ssl/my.cert");

    // Configure and start the TCP listener
    HttpListener *httplistener=new HttpListener(settings,new RequestHandler(&app),&app);

    // Check header "x-version" for value "1"
    const auto versionValidator=[](const stefanfrings::Headers &headersMap)->stefanfrings::HeadersCheckingStatus {
        bool isOk = true;

        const auto version=headersMap.value("x-version", "-1").toInt(&isOk);

        if (!isOk)
            return {false, {406, "Bad version!"}};

        if (version <= 0)
            return {false, {406, "No version in headers!"}};

        if (version > 2)
            return {false, {422, "Version is too high!"}};

        return { true, {}};
    };

    // Check "authorization" header for "Basic dXNlcjpwYXNzd29yZA=="
    const auto authenticationValidator=[](const stefanfrings::Headers &headersMap)->stefanfrings::HeadersCheckingStatus {
        const auto authorization=QString{headersMap.value("authorization")};

        if (authorization.isEmpty())
            return {false, {403, "No authorization header!"}};

        if (!authorization.startsWith("Basic "))
            return {false, {403, "Wrong authorization header!"}};

        const auto userCredentials=QByteArray("user:password").toBase64();
        const auto rightAuthorization=QByteArray("Basic ")+userCredentials;

        if (authorization!=rightAuthorization)
            return {false, {403, "Wrong password!"}};

        return { true, {} };
    };

    stefanfrings::HttpError err;
    httplistener->setHeadersHandler({{versionValidator, authenticationValidator}, err});

    qWarning("Application has started");
    app.exec();
    qWarning("Application has stopped");
}

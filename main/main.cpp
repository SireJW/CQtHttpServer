/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtHttpServer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
//#include "tst_qhttpserver.cpp"
#include <QHostAddress>
#include "httpserver/qhttpserver.h"
#include "httpserver/qhttpserverrequest.h"
#include "httpserver/qhttpserverrouterrule.h"

#include "httpserver/qhttpserverrouterrule_p.h"
#include "httpserver/qhttpserverliterals_p.h"

#include <QJsonDocument>

class QueryRequireRouterRule : public QHttpServerRouterRule
{
public:
    QueryRequireRouterRule(const QString &pathPattern,
                           const char *queryKey,
                           RouterHandler &&routerHandler)
        : QHttpServerRouterRule(pathPattern, std::forward<RouterHandler>(routerHandler)),
          m_queryKey(queryKey)
    {
    }

    bool matches(const QHttpServerRequest &request, QRegularExpressionMatch *match) const override
    {
        if (QHttpServerRouterRule::matches(request, match)) {
            if (request.query().hasQueryItem(m_queryKey))
                return true;
        }

        return false;
    }

private:
    const char * m_queryKey;
};

struct CustomArg {
    int data = 10;

    CustomArg() {}
    CustomArg(const QString &urlArg) : data(urlArg.toInt()) {}
};
Q_DECLARE_METATYPE(CustomArg)


QByteArray toJsonByteArray(const QVariant &var)
{
    return QJsonDocument::fromVariant(var).toJson();
}
QVariant jsonToVariant(const QByteArray &b)
{
    return QJsonDocument::fromJson(b).toVariant();
}

static inline QString host(const QHttpServerRequest &request)
{
    return request.headers()[QStringLiteral("Host")].toString();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QHttpServer httpserver;
    QString urlBase;
    QString sslUrlBase;
    //http://localhost:7109/socket.io/?EIO=3&transport=polling&t=N5yWtST
    httpserver.route("/socket.io/?", [] (QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {
        const auto &role = req.query().queryItemValue(QString::fromLatin1("role"));
        const auto &fragment = req.url().fragment();
        qDebug()<<QString("1111role=%1#%2'").arg(role, fragment);

        QVariantMap map = req.headers();
        QString transport = map.value("transport").toString();
        QString sid = map.value("sid").toString();
        qDebug()<<"transport:"<<transport<<"sid:"<<sid;
        resp.write();
        //        return QString(" role=%1#%2'").arg(role, fragment);
    });

    httpserver.route("/api/c/ping",[](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req){
        QVariantMap header = req.headers();
        QString cookie = header.value("cookie").toString();
        qDebug()<<"cookie:"<<cookie;
        QString userId = header.value("UserId").toString();
        QString userToken = header.value("UserToken").toString();
        qDebug()<<"userId:"<<userId;
        qDebug()<<"userToken:"<<userToken;

        QVariantMap map;
        map.insert("id",userId);
        map.insert("username","admin");
        map.insert("admin",true);
        QByteArray jsonArray = toJsonByteArray(map);

        resp.write(jsonArray,"application/json",QHttpServerResponder::StatusCode::Ok);
    });
    //http://localhost:7109/static/assets/error.wav?v=65594f7
    httpserver.route("static/assets",[](const QString &str,QHttpServerResponder &&resp,
                     const QHttpServerRequest &req){
        qDebug()<<"/static/assets:"<<str;
        QFile file(QString("html/static/assets/%1").arg(str));
        qDebug()<<"file1:"<<file.fileName();
        if(!file.open(QIODevice::ReadOnly)){
            return;
        }
        resp.write(file.readAll(),"wav",QHttpServerResponder::StatusCode::Ok);
        file.close();
    });
    httpserver.route("/api/c/sign-in",[](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req){
        resp.writeHeader("UserId","1111111111111");
        resp.writeHeader("UserToken","22222222222222222");
        resp.write();
    });
    httpserver.route("/api/c/sign-out",[](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req){
        QVariantMap map;
        resp.write();
    });
    httpserver.route("/api/c/permission-defs",[](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req){
        QVariantMap map;
        resp.write();
    });
    httpserver.route("/static/index.4f10d0b355e26f0721e7.css", [](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {

        QFile file(QString("html/static/%1").arg(req.url().fileName()));
        qDebug()<<"file0:"<<file.fileName();
        if(!file.open(QIODevice::ReadOnly)){
            return;
        }
        resp.write(file.readAll(),"text/css",QHttpServerResponder::StatusCode::Ok);
        file.close();
    });
    httpserver.route("/static/img/", [](const QString &str,QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {
        qDebug()<<"/static/img:"<<str;
        QFile file(QString("html/static/img/%1").arg(str));
        qDebug()<<"file1:"<<file.fileName();
        if(!file.open(QIODevice::ReadOnly)){
            return;
        }
        resp.write(file.readAll(),"jpg",QHttpServerResponder::StatusCode::Ok);
        file.close();
    });
    httpserver.route("/static/index.4f10d0b355e26f0721e7.bundle.js", [](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {

        QFile file(QString("html/static/%1").arg(req.url().fileName()));
        qDebug()<<"file1:"<<file.fileName();
        if(!file.open(QIODevice::ReadOnly)){
            return;
        }
        resp.write(file.readAll(),"text/js",QHttpServerResponder::StatusCode::Ok);
        file.close();
    });
    httpserver.route("/index.html", [](QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {
        QFile file("html/index.html");
        if(!file.open(QIODevice::ReadOnly)){
            return;
        }
        resp.write(file.readAll(),QHttpServerLiterals::contentTypeTextHtml(),QHttpServerResponder::StatusCode::Ok);
        file.close();
    });
    httpserver.route("/req-and-resp", [] (QHttpServerResponder &&resp,
                     const QHttpServerRequest &req) {
        resp.write(req.body(),
                   QHttpServerLiterals::contentTypeTextHtml());
    });

    httpserver.route("/resp-and-req", [] (const QHttpServerRequest &req,
                     QHttpServerResponder &&resp) {
        resp.write(req.body(),
                   QHttpServerLiterals::contentTypeTextHtml());
    });

    httpserver.route("/test", [] (QHttpServerResponder &&responder) {
        responder.write("test msg",
                        QHttpServerLiterals::contentTypeTextHtml());
    });

    httpserver.route("/", QHttpServerRequest::Method::Get, [] () {
        return "Hello world get";
    });

    httpserver.route("/", QHttpServerRequest::Method::Post, [] () {
        return "Hello world post";
    });

    httpserver.route("/post-and-get", "GET|POST", [] (const QHttpServerRequest &request) {
        if (request.method() == QHttpServerRequest::Method::Get)
            return "Hello world get";
        else if (request.method() == QHttpServerRequest::Method::Post)
            return "Hello world post";

        return "This should not work";
    });

    httpserver.route("/any", "All", [] (const QHttpServerRequest &request) {
        static const auto metaEnum = QMetaEnum::fromType<QHttpServerRequest::Method>();
        return metaEnum.valueToKey(static_cast<int>(request.method()));
    });

    httpserver.route("/page/", [] (const qint32 number) {
        return QString("page: %1").arg(number);
    });

    httpserver.route("/page/<arg>/detail", [] (const quint32 number) {
        return QString("page: %1 detail").arg(number);
    });

    httpserver.route("/user/", [] (const QString &name) {
        return QString("%1").arg(name);
    });

    httpserver.route("/user/<arg>/", [] (const QString &name, const QByteArray &ba) {
        return QString("%1-%2").arg(name).arg(QString::fromLatin1(ba));
    });

    httpserver.route("/test/", [] (const QUrl &url) {
        return QString("path: %1").arg(url.path());
    });

    httpserver.route("/api/v", [] (const float api) {
        return QString("api %1v").arg(api);
    });

    httpserver.route("/api/v<arg>/user/", [] (const float api, const quint64 user) {
        return QString("api %1v, user id - %2").arg(api).arg(user);
    });

    httpserver.route("/api/v<arg>/user/<arg>/settings", [] (const float api, const quint64 user,
                     const QHttpServerRequest &request) {
        const auto &role = request.query().queryItemValue(QString::fromLatin1("role"));
        const auto &fragment = request.url().fragment();

        return QString("api %1v, user id - %2, set settings role=%3#'%4'")
                .arg(api).arg(user).arg(role, fragment);
    });

    httpserver.route<QueryRequireRouterRule>(
                "/custom/",
                "key",
                [] (const quint64 num, const QHttpServerRequest &request) {
        return QString("Custom router rule: %1, key=%2")
                .arg(num)
                .arg(request.query().queryItemValue("key"));
    });

//    httpserver.router()->addConverter<CustomArg>(QString("[+-]?\\d+"));
//    httpserver.route("/socket.io", [] (const CustomArg &customArg) {
//        return QString("data = %1").arg(customArg.data);
//    });

    httpserver.route("/post-body", "POST", [] (const QHttpServerRequest &request) {
        return request.body();
    });

    httpserver.route("/file/", [] (const QString &file) {
        return QHttpServerResponse::fromFile((QString("data/") + file));
    });

    httpserver.route("/json-object/", [] () {
        return QJsonObject{
            {"property", "test"},
            {"value", 1}
        };
    });

    httpserver.route("/json-array/", [] () {
        return QJsonArray{
            1, "2",
            QJsonObject{
                {"name", "test"}
            }
        };
    });

    httpserver.route("/chunked/", [] (QHttpServerResponder &&responder) {
        responder.writeStatusLine(QHttpServerResponder::StatusCode::Ok);
        responder.writeHeaders({
                                   {"Content-Type", "text/plain"},
                                   {"Transfer-Encoding", "chunked"} });

        auto writeChunk = [&responder] (const char *message) {
            responder.writeBody(QByteArray::number(qstrlen(message), 16));
            responder.writeBody("\r\n");
            responder.writeBody(message);
            responder.writeBody("\r\n");
        };

        writeChunk("part 1 of the message, ");
        writeChunk("part 2 of the message");
        writeChunk("");
    });

    httpserver.route("/extra-headers", [] () {
        QHttpServerResponse resp("");
        resp.setHeader("Content-Type", "application/x-empty");
        resp.setHeader("Server", "test server");
        return resp;
    });

    quint16 port = httpserver.listen(QHostAddress::Any,7109);
    if (!port)
        qCritical() << "Http server listen failed";

    urlBase = QStringLiteral("http://localhost:%1").arg(port);
    qDebug()<<"urlBase:"<<urlBase;
    //#if QT_CONFIG(ssl)
    //    httpserver.sslSetup(QSslCertificate(g_certificate),
    //                        QSslKey(g_privateKey, QSsl::Rsa));

    //    port = httpserver.listen();
    //    if (!port)
    //        qCritical() << "Http server listen failed";

    //    sslUrlBase = QStringLiteral("https://localhost:%1%2").arg(port);

    //    QList<QSslError> expectedSslErrors;

    //// Non-OpenSSL backends are not able to report a specific error code
    //// for self-signed certificates.
    //#ifndef QT_NO_OPENSSL
    //# define FLUKE_CERTIFICATE_ERROR QSslError::SelfSignedCertificate
    //#else
    //# define FLUKE_CERTIFICATE_ERROR QSslError::CertificateUntrusted
    //#endif

    //    expectedSslErrors.append(QSslError(FLUKE_CERTIFICATE_ERROR,
    //                                       QSslCertificate(g_certificate)));
    //    expectedSslErrors.append(QSslError(QSslError::HostNameMismatch,
    //                                       QSslCertificate(g_certificate)));

    //    connect(&networkAccessManager, &QNetworkAccessManager::sslErrors,
    //            [expectedSslErrors](QNetworkReply *reply,
    //                                const QList<QSslError> &errors) {
    //        for (const auto &error: errors) {
    //            for (const auto &expectedError: expectedSslErrors) {
    //                if (error.error() != expectedError.error() ||
    //                    error.certificate() != expectedError.certificate()) {
    //                    qCritical() << "Got unexpected ssl error:"
    //                                << error << error.certificate();
    //                }
    //            }
    //        }
    //        reply->ignoreSslErrors(expectedSslErrors);
    //    });
    //#endif
    //    httpServer.route("/index.html", [](QHttpServerResponder &&resp,
    //                     const QHttpServerRequest &req) {
    //        //        write(data,
    //        //              {{ QHttpServerLiterals::contentTypeHeader(), mimeType }},
    //        //              status);
    //        {
    //            QFile file("html/index.4f10d0b355e26f0721e7.bundle.js");
    //            if(!file.open(QIODevice::ReadOnly)){
    //                return;
    //            }
    //            resp.write(file.readAll(),QByteArray("text/js"),QHttpServerResponder::StatusCode::Ok);
    //        }
    //        {
    //            QFile file("html/index.4f10d0b355e26f0721e7.css");
    //            if(!file.open(QIODevice::ReadOnly)){
    //                return;
    //            }
    //            resp.write(file.readAll(),QByteArray("text/css"),QHttpServerResponder::StatusCode::Ok);
    //        }
    //        QFile file("html/index.html");
    //        if(!file.open(QIODevice::ReadOnly)){
    //            return;
    //        }
    //        resp.write(file.readAll(),QHttpServerLiterals::contentTypeTextHtml(),QHttpServerResponder::StatusCode::Ok);
    //    });

    //    httpServer.route("/socket.io", [](QHttpServerResponder &&resp) {

    //        //        QString transport = queryMap.value("transport");
    //        //        QString sid = queryMap.value("sid");
    //        //        qDebug()<<"transport:"<<transport<<"sid:"<<sid;
    //        //        ws://localhost:7106/socket.io/?EIO=3&transport=websocket&sid=bxePO_fcfB1qbYabAAAA
    //        //回复一下 socket.io
    //        //        ws://localhost:7106/socket.io/?EIO=3&transport=websocket&sid=2AVSiKpwSk6wPD88AAAB
    //        QString json = "["
    //                       "\"rbk-client-onTcpConnected\",{\"ip\":\"127.0.0.1\",\"port\":19204,\"type\":1000,\"number\":100,\"jsonData\":\"\",\"reqHex\":\"5a0100640000000003e8000000000000\"}]";
    //        //        resp.writeBody(json.toLocal8Bit());
    //        resp.write(QHttpServerResponder::StatusCode::Ok);
    //    });
    //    httpServer.route("/api/c/ping", [](QHttpServerResponder &&resp) {
    //        QVariantMap map;
    //        map.insert("id","123");
    //        map.insert("username","admin");
    //        map.insert("admin",true);
    //        resp.write(QHttpServerResponder::StatusCode::Ok);
    //        //        resp.write(QJsonDocument::fromVariant(map).toJson(),req.headers().toStdMap());

    //    });
    //    httpServer.route("/api/c/sign-in", [](QHttpServerResponder &&resp) {
    //        resp.write(QHttpServerResponder::StatusCode::Ok);
    //    });
    //    httpServer.route("/api/c/sign-out", [](QHttpServerResponder &&resp) {
    //        resp.write(QHttpServerResponder::StatusCode::Ok);
    //    });
    //    httpServer.route("/api/c/permission-defs", [](QHttpServerResponder &&resp) {
    //        resp.write(QHttpServerResponder::StatusCode::Ok);
    //    });
    //    //    httpServer.route("/", []() {
    //    //        return "Hello world";
    //    //    });

    //    //    httpServer.route("/query", [] (const QHttpServerRequest &request) {
    //    //        return QString("%1/query/").arg(host(request));
    //    //    });

    //    //    httpServer.route("/query/", [] (qint32 id, const QHttpServerRequest &request) {
    //    //        return QString("%1/query/%2").arg(host(request)).arg(id);
    //    //    });

    //    //    httpServer.route("/query/<arg>/log", [] (qint32 id, const QHttpServerRequest &request) {
    //    //        return QString("%1/query/%2/log").arg(host(request)).arg(id);
    //    //    });

    //    //    httpServer.route("/query/<arg>/log/", [] (qint32 id, float threshold,
    //    //                                              const QHttpServerRequest &request) {
    //    //        return QString("%1/query/%2/log/%3").arg(host(request)).arg(id).arg(threshold);
    //    //    });

    //    //    httpServer.route("/user/", [] (const qint32 id) {
    //    //        return QString("User %1").arg(id);
    //    //    });

    //    //    httpServer.route("/user/<arg>/detail", [] (const qint32 id) {
    //    //        return QString("User %1 detail").arg(id);
    //    //    });

    //    //    httpServer.route("/user/<arg>/detail/", [] (const qint32 id, const qint32 year) {
    //    //        return QString("User %1 detail year - %2").arg(id).arg(year);
    //    //    });

    //    //    httpServer.route("/json/", [] {
    //    //        return QJsonObject{
    //    //            {
    //    //                {"key1", "1"},
    //    //                {"key2", "2"},
    //    //                {"key3", "3"}
    //    //            }
    //    //        };
    //    //    });

    //    //    httpServer.route("/assets/<arg>", [] (const QUrl &url) {
    //    //        return QHttpServerResponse::fromFile(QStringLiteral(":/assets/%1").arg(url.path()));
    //    //    });

    //    //    httpServer.route("/remote_address", [](const QHttpServerRequest &request) {
    //    //        return request.remoteAddress().toString();
    //    //    });

    //    const auto port = httpServer.listen(QHostAddress::Any,7109);
    //    if (!port) {
    //        qDebug() << QCoreApplication::translate(
    //                        "QHttpServerExample", "Server failed to listen on a port.");
    //        return 0;
    //    }

    //    qDebug() << QCoreApplication::translate(
    //                    "QHttpServerExample", "Running on http://127.0.0.1:%1/ (Press CTRL+C to quit)").arg(port);

    return app.exec();
}


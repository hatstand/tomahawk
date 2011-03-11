#ifndef SCRIPTRESOLVER_H
#define SCRIPTRESOLVER_H

#include <QProcess>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "resolver.h"
#include "query.h"
#include "result.h"

class ScriptResolver : public Tomahawk::Resolver
{
Q_OBJECT
public:
    explicit ScriptResolver(const QString& exe);
    virtual ~ScriptResolver();
    
    virtual QString name() const            { return m_name; }
    virtual unsigned int weight() const     { return m_weight; }
    virtual unsigned int preference() const { return m_preference; }
    virtual unsigned int timeout() const    { return m_timeout; }

    virtual void resolve( const Tomahawk::query_ptr& query );

    void stop();
    
    QString exe() const { return m_cmd; }
signals:
    void finished();
    
public slots:

private slots:
    void readStderr();
    void readStdout();
    void cmdExited(int code, QProcess::ExitStatus status);

private:
    void handleMsg( const QByteArray& msg );
    void sendMsg( const QByteArray& msg );
    void doSetup( const QVariantMap& m );

    QProcess m_proc;
    QString m_name, m_cmd;
    unsigned int m_weight, m_preference, m_timeout, m_num_restarts;

    quint32 m_msgsize;
    QByteArray m_msg;

    bool m_ready, m_stopped;

    QJson::Parser m_parser;
    QJson::Serializer m_serializer;

};

#endif // SCRIPTRESOLVER_H

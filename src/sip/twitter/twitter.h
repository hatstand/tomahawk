#ifndef TWITTER_H
#define TWITTER_H

#include "twitterconfigwidget.h"

#include <QTimer>
#include <QWeakPointer>
#include <QSet>

#include <qtweetuser.h>
#include <qtweetnetbase.h>
#include <qtweetfriendstimeline.h>
#include <qtweetdirectmessages.h>
#include <qtweetdirectmessagenew.h>
#include <qtweetdirectmessagedestroy.h>
#include <qtweetmentions.h>
#include <qtweetdmstatus.h>

#include "../sipdllmacro.h"
#include "sip/SipPlugin.h"
#include "tomahawkoauthtwitter.h"

#define MYNAME "SIPTWITTER"

class SIPDLLEXPORT TwitterPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )
    
public:
    TwitterPlugin();

    virtual ~TwitterPlugin() {}
    
    virtual bool isValid();
    virtual const QString name();
    virtual const QString accountName();
    virtual const QString friendlyName();

    virtual QWidget* configWidget();

public slots:
    virtual bool connectPlugin( bool startup );

    void disconnectPlugin();

    void sendMsg( const QString& to, const QString& msg )
    {
    }

    void broadcastMsg( const QString &msg )
    {
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
    }

private slots:
    void connectAuthVerifyReply( const QTweetUser &user );
    void checkTimerFired();
    void connectTimerFired();
    void friendsTimelineStatuses( const QList< QTweetStatus > &statuses );
    void mentionsStatuses( const QList< QTweetStatus > &statuses );
    void pollDirectMessages();
    void directMessages( const QList< QTweetDMStatus > &messages );
    void directMessagePosted( const QTweetDMStatus &message );
    void directMessagePostError( QTweetNetBase::ErrorCode errorCode, const QString &message );
    void directMessageDestroyed( const QTweetDMStatus &message );
    void registerOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata );
    void sendOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata );
    void makeConnection( const QString &screenName, const QHash< QString, QVariant > &peerdata );

private:
    QWeakPointer< TomahawkOAuthTwitter > m_twitterAuth;
    QWeakPointer< QTweetFriendsTimeline > m_friendsTimeline;
    QWeakPointer< QTweetMentions > m_mentions;
    QWeakPointer< QTweetDirectMessages > m_directMessages;
    QWeakPointer< QTweetDirectMessageNew > m_directMessageNew;
    QWeakPointer< QTweetDirectMessageDestroy > m_directMessageDestroy;
    bool m_isAuthed;
    bool m_isOnline;
    QTimer m_checkTimer;
    QTimer m_connectTimer;
    qint64 m_cachedFriendsSinceId;
    qint64 m_cachedMentionsSinceId;
    qint64 m_cachedDirectMessagesSinceId;
    QHash< QString, QVariant > m_cachedPeers;
    QSet<QString> m_keyCache;
    bool m_finishedFriends;
    bool m_finishedMentions;

    TwitterConfigWidget *m_configWidget;
};

#endif

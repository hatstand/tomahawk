#include "databasecommand_deletefiles.h"

#include <QSqlQuery>

#include "artist.h"
#include "album.h"
#include "collection.h"
#include "database/database.h"
#include "databasecommand_collectionstats.h"
#include "databaseimpl.h"
#include "network/controlconnection.h"

using namespace Tomahawk;


// After changing a collection, we need to tell other bits of the system:
void
DatabaseCommand_DeleteFiles::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;

    // make the collection object emit its tracksAdded signal, so the
    // collection browser will update/fade in etc.
    Collection* coll = source()->collection().data();

    connect( this, SIGNAL( notify( QStringList, Tomahawk::collection_ptr ) ),
             coll,   SLOT( delTracks( QStringList, Tomahawk::collection_ptr ) ), Qt::QueuedConnection );

    emit notify( m_files, source()->collection() );

    // also re-calc the collection stats, to updates the "X tracks" in the sidebar etc:
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( source() );
    connect( cmd,            SIGNAL( done( QVariantMap ) ),
             source().data(),  SLOT( setStats( QVariantMap ) ), Qt::QueuedConnection );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_DeleteFiles::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    int deleted = 0;
    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();
    TomahawkSqlQuery delquery = dbi->newquery();

    if ( !m_dir.path().isEmpty() && source()->isLocal() )
    {
        qDebug() << "Deleting" << m_dir.path() << "from db for localsource" << srcid;
        TomahawkSqlQuery dirquery = dbi->newquery();

        dirquery.prepare( QString( "SELECT id, url FROM file WHERE source %1 AND url LIKE ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );
        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND id = ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

        dirquery.bindValue( 0, "file://" + m_dir.absolutePath() + "/%" );
        dirquery.exec();

        while ( dirquery.next() )
        {
            QFileInfo fi( dirquery.value( 1 ).toString().mid( 7 ) ); // remove file://
            if ( fi.absolutePath() != m_dir.absolutePath() )
            {
                qDebug() << "Skipping subdir:" << fi.absolutePath();
                continue;
            }

            m_ids << dirquery.value( 0 ).toUInt();
            m_files << dirquery.value( 1 ).toString();
        }

        foreach ( const QVariant& id, m_ids )
        {
            delquery.bindValue( 0, id.toUInt() );
            if( !delquery.exec() )
            {
                qDebug() << "Failed to delete file:"
                    << delquery.lastError().databaseText()
                    << delquery.lastError().driverText()
                    << delquery.boundValues();
                continue;
            }

            deleted++;
        }
    }
    else
    {
        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND url = ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

        foreach( const QVariant& id, m_ids )
        {
            qDebug() << "Deleting" << id.toUInt() << "from db for source" << srcid;

            const QString url = QString( "servent://%1\t%2" ).arg( source()->userName() ).arg( id.toString() );
            m_files << url;

            delquery.bindValue( 0, id.toUInt() );
            if( !delquery.exec() )
            {
                qDebug() << "Failed to delete file:"
                    << delquery.lastError().databaseText()
                    << delquery.lastError().driverText()
                    << delquery.boundValues();
                continue;
            }
            
            deleted++;
        }
    }
    
    qDebug() << "Deleted" << deleted << m_ids << m_files;

    emit done( m_files, source()->collection() );
}

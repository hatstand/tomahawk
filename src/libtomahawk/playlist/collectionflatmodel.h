#ifndef COLLECTIONFLATMODEL_H
#define COLLECTIONFLATMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QHash>

#include "plitem.h"
#include "trackmodel.h"
#include "collection.h"
#include "query.h"
#include "typedefs.h"
#include "playlist.h"
#include "playlistinterface.h"

#include "database/databasecommand_alltracks.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT CollectionFlatModel : public TrackModel
{
Q_OBJECT

public:
    explicit CollectionFlatModel( QObject* parent = 0 );
    ~CollectionFlatModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    virtual int trackCount() const { return rowCount( QModelIndex() ) + m_tracksToAdd.count(); }

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    void addCollection( const Tomahawk::collection_ptr& collection );
    void removeCollection( const Tomahawk::collection_ptr& collection );

    void addFilteredCollection( const Tomahawk::collection_ptr& collection, unsigned int amount, DatabaseCommand_AllTracks::SortOrder order );

    virtual void append( const Tomahawk::query_ptr& query ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

private slots:
    void onDataChanged();

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );
    void onTracksRemoved( const QList<Tomahawk::query_ptr>& tracks );

    void onSourceOffline( const Tomahawk::source_ptr& src );

    void processTracksToAdd();

private:
    QMap< Tomahawk::collection_ptr, QPair< int, int > > m_collectionRows;
    QList<Tomahawk::query_ptr> m_tracksToAdd;
};

#endif // COLLECTIONFLATMODEL_H

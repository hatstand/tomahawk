#ifndef PLITEM_H
#define PLITEM_H

#include <QHash>
#include <QVector>
#include <QPersistentModelIndex>
#include <QAbstractItemModel>

#include "query.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT PlItem : public QObject
{
Q_OBJECT

public:
    virtual ~PlItem();

    explicit PlItem( PlItem* parent = 0, QAbstractItemModel* model = 0 );
    explicit PlItem( const QString& caption, PlItem* parent = 0 );
    explicit PlItem( const Tomahawk::query_ptr& query, PlItem* parent = 0, int row = -1 );
    explicit PlItem( const Tomahawk::plentry_ptr& entry, PlItem* parent = 0, int row = -1 );

    const Tomahawk::plentry_ptr& entry() const;
    const Tomahawk::query_ptr& query() const;

    bool isPlaying() { return m_isPlaying; }
    void setIsPlaying( bool b ) { m_isPlaying = b; emit dataChanged(); }

    PlItem* parent;
    QVector<PlItem*> children;
    QHash<QString, PlItem*> hash;
    QString caption;
    int childCount;
    QPersistentModelIndex index;
    QAbstractItemModel* model;
    bool toberemoved;

signals:
    void dataChanged();

private:
    void setupItem( const Tomahawk::query_ptr& query, PlItem* parent, int row = -1 );

    Tomahawk::plentry_ptr m_entry;
    Tomahawk::query_ptr m_query;
    bool m_isPlaying;
};

#endif // PLITEM_H

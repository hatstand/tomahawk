#include "playlistmanager.h"

#include <QVBoxLayout>

#include "audio/audioengine.h"
#include "utils/animatedsplitter.h"
#include "infobar/infobar.h"
#include "topbar/topbar.h"
#include "widgets/infowidgets/sourceinfowidget.h"
#include "widgets/welcomewidget.h"

#include "collectionmodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "queueview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"
#include "albumview.h"
#include "albumproxymodel.h"
#include "albummodel.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "utils/widgetdragfilter.h"

#include "dynamic/widgets/DynamicWidget.h"

#include "widgets/welcomewidget.h"
#include "widgets/infowidgets/sourceinfowidget.h"

#define FILTER_TIMEOUT 280

using namespace Tomahawk;

PlaylistManager* PlaylistManager::s_instance = 0;


PlaylistManager*
PlaylistManager::instance()
{
    return s_instance;
}


PlaylistManager::PlaylistManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QWidget() )
    , m_welcomeWidget( new WelcomeWidget() )
    , m_currentMode( 0 )
{
    s_instance = this;

    setHistoryPosition( -1 );
    m_widget->setLayout( new QVBoxLayout() );

    m_topbar = new TopBar();
    m_infobar = new InfoBar();
    m_stack = new QStackedWidget();

    m_infobar->installEventFilter( new WidgetDragFilter( m_infobar ) );

    QFrame* line = new QFrame();
    line->setFrameStyle( QFrame::HLine );
    line->setStyleSheet( "border: 1px solid gray;" );
    line->setMaximumHeight( 1 );

    m_splitter = new AnimatedSplitter();
    m_splitter->setOrientation( Qt::Vertical );
    m_splitter->setChildrenCollapsible( false );
    m_splitter->setGreedyWidget( 0 );
    m_splitter->addWidget( m_stack );

    m_queueView = new QueueView( m_splitter );
    m_queueModel = new PlaylistModel( m_queueView );
    m_queueView->queue()->setModel( m_queueModel );
    AudioEngine::instance()->setQueue( m_queueView->queue()->proxyModel() );

    m_splitter->addWidget( m_queueView );
    m_splitter->hide( 1, false );

    m_widget->layout()->addWidget( m_infobar );
    m_widget->layout()->addWidget( m_topbar );
    m_widget->layout()->addWidget( line );
    m_widget->layout()->addWidget( m_splitter );

    m_superCollectionView = new CollectionView();
    m_superCollectionFlatModel = new CollectionFlatModel( m_superCollectionView );
    m_superCollectionView->setModel( m_superCollectionFlatModel );
    m_superCollectionView->setFrameShape( QFrame::NoFrame );
    m_superCollectionView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_superAlbumView = new AlbumView();
    m_superAlbumModel = new AlbumModel( m_superAlbumView );
    m_superAlbumView->setModel( m_superAlbumModel );
    m_superAlbumView->setFrameShape( QFrame::NoFrame );
    m_superAlbumView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    
    m_stack->setContentsMargins( 0, 0, 0, 0 );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->setSpacing( 0 );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );

    connect( m_topbar, SIGNAL( filterTextChanged( QString ) ),
                         SLOT( setFilter( QString ) ) );

    connect( m_topbar, SIGNAL( flatMode() ),
                         SLOT( setTableMode() ) );
    
    connect( m_topbar, SIGNAL( artistMode() ),
                         SLOT( setTreeMode() ) );
    
    connect( m_topbar, SIGNAL( albumMode() ),
                         SLOT( setAlbumMode() ) );
}


PlaylistManager::~PlaylistManager()
{
    delete m_widget;
}


PlaylistView*
PlaylistManager::queue() const
{
    return m_queueView->queue();
}


bool
PlaylistManager::show( const Tomahawk::playlist_ptr& playlist )
{
    PlaylistView* view;
    if ( !m_playlistViews.contains( playlist ) )
    {
        view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->loadPlaylist( playlist );
        playlist->resolve();

        m_playlistViews.insert( playlist, view );
    }
    else
    {
        view = m_playlistViews.value( playlist );
    }
    
    setPage( view );
    TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( playlist );
    emit numSourcesChanged( SourceList::instance()->count() );

    return true;
}


bool 
PlaylistManager::show( const Tomahawk::dynplaylist_ptr& playlist )
{
    if ( !m_dynamicWidgets.contains( playlist ) )
    {
       m_dynamicWidgets[ playlist ] = new Tomahawk::DynamicWidget( playlist, m_stack );

       playlist->resolve();
    }
    
    setPage( m_dynamicWidgets.value( playlist ) );

    if ( playlist->mode() == Tomahawk::OnDemand )
        m_queueView->hide();
    else
        m_queueView->show();
    
    TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( playlist );
    emit numSourcesChanged( SourceList::instance()->count() );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::artist_ptr& artist )
{
    PlaylistView* view;

    if ( !m_artistViews.contains( artist ) )
    {
        view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( artist );

        m_artistViews.insert( artist, view );
    }
    else
    {
        view = m_artistViews.value( artist );
    }
    
    setPage( view );
    emit numSourcesChanged( 1 );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::album_ptr& album )
{
    PlaylistView* view;
    if ( !m_albumViews.contains( album ) )
    {
        view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( album );

        m_albumViews.insert( album, view );
    }
    else
    {
        view = m_albumViews.value( album );
    }
    
    setPage( view );
    emit numSourcesChanged( 1 );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::collection_ptr& collection )
{
    m_currentCollection = collection;
    if ( m_currentMode == 0 )
    {
        CollectionView* view;
        if ( !m_collectionViews.contains( collection ) )
        {
            view = new CollectionView();
            CollectionFlatModel* model = new CollectionFlatModel();
            view->setModel( model );
            view->setFrameShape( QFrame::NoFrame );
            view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
            model->addCollection( collection );

            m_collectionViews.insert( collection, view );
        }
        else
        {
            view = m_collectionViews.value( collection );
        }

        setPage( view );
    }

    if ( m_currentMode == 2 )
    {
        AlbumView* aview;
        if ( !m_collectionAlbumViews.contains( collection ) )
        {
            aview = new AlbumView();
            AlbumModel* amodel = new AlbumModel( aview );
            aview->setModel( amodel );
            aview->setFrameShape( QFrame::NoFrame );
            aview->setAttribute( Qt::WA_MacShowFocusRect, 0 );
            amodel->addCollection( collection );

            m_collectionAlbumViews.insert( collection, aview );
        }
        else
        {
            aview = m_collectionAlbumViews.value( collection );
        }

        setPage( aview );
    }

    emit numSourcesChanged( 1 );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::source_ptr& source )
{
    SourceInfoWidget* swidget;
    if ( !m_sourceViews.contains( source ) )
    {
        swidget = new SourceInfoWidget( source );
        m_sourceViews.insert( source, swidget );
    }
    else
    {
        swidget = m_sourceViews.value( source );
    }

    setPage( swidget );
    emit numSourcesChanged( 1 );

    return true;
}


bool
PlaylistManager::show( ViewPage* page )
{
    if ( m_stack->indexOf( page->widget() ) < 0 )
    {
        connect( page->widget(), SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ) );
    }

    setPage( page );

    return true;
}


bool
PlaylistManager::showSuperCollection()
{
    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
    {
        if ( !m_superCollections.contains( source->collection() ) )
        {
            m_superCollections.append( source->collection() );
            m_superCollectionFlatModel->addCollection( source->collection() );
            m_superAlbumModel->addCollection( source->collection() );
        }

        m_superCollectionFlatModel->setTitle( tr( "All available tracks" ) );
        m_superAlbumModel->setTitle( tr( "All available albums" ) );
    }

    if ( m_currentMode == 0 )
    {
        setPage( m_superCollectionView );
    }
    else if ( m_currentMode == 2 )
    {
        setPage( m_superAlbumView );
    }

    emit numSourcesChanged( m_superCollections.count() );

    return true;
}


void
PlaylistManager::showWelcomePage()
{
    show( m_welcomeWidget );
}


void
PlaylistManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 0;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::setTreeMode()
{
    return;

    qDebug() << Q_FUNC_INFO;

    m_currentMode = 1;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::setAlbumMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 2;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::showQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "showQueue", Qt::QueuedConnection );
        return;
    }

    m_splitter->show( 1 );
}


void
PlaylistManager::hideQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "hideQueue", Qt::QueuedConnection );
        return;
    }

    m_splitter->hide( 1 );
}


void
PlaylistManager::historyBack()
{
    if ( m_historyPosition < 1 )
        return;

    showHistory( m_historyPosition - 1 );
}


void
PlaylistManager::historyForward()
{
    if ( m_historyPosition >= m_pageHistory.count() - 1 )
        return;
    
    showHistory( m_historyPosition + 1 );
}


void
PlaylistManager::showHistory( int historyPosition )
{
    if ( historyPosition < 0 || historyPosition >= m_pageHistory.count() )
    {
        qDebug() << "History position out of bounds!" << historyPosition << m_pageHistory.count();
        Q_ASSERT( false );
        return;
    }

    setHistoryPosition( historyPosition );
    ViewPage* page = m_pageHistory.at( historyPosition );
    setPage( page, false );
}


void
PlaylistManager::setFilter( const QString& filter )
{
    m_filter = filter;

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
PlaylistManager::applyFilter()
{
    qDebug() << Q_FUNC_INFO;

    if ( currentPlaylistInterface() && currentPlaylistInterface()->filter() != m_filter )
        currentPlaylistInterface()->setFilter( m_filter );
}


void
PlaylistManager::setPage( ViewPage* page, bool trackHistory )
{
    unlinkPlaylist();

    if ( !m_pageHistory.contains( page ) )
    {
        m_stack->addWidget( page->widget() );
    }
    else
    {
        if ( trackHistory )
            m_pageHistory.removeAll( page );
    }

    if ( trackHistory )
    {
        m_pageHistory << page;
        setHistoryPosition( m_pageHistory.count() - 1 );
    }

    if ( playlistForInterface( currentPlaylistInterface() ) )
        emit playlistActivated( playlistForInterface( currentPlaylistInterface() ) );
    if ( dynamicPlaylistForInterface( currentPlaylistInterface() ) )
        emit dynamicPlaylistActivated( dynamicPlaylistForInterface( currentPlaylistInterface() ) );
    if ( collectionForInterface( currentPlaylistInterface() ) )
        emit collectionActivated( collectionForInterface( currentPlaylistInterface() ) );
    if ( isSuperCollectionVisible() )
        emit superCollectionActivated();
    if ( !currentPlaylistInterface() )
        emit tempPageActivated();

    if ( !AudioEngine::instance()->isPlaying() )
        AudioEngine::instance()->setPlaylist( currentPlaylistInterface() );

    m_stack->setCurrentWidget( page->widget() );
    updateView();
}


void
PlaylistManager::unlinkPlaylist()
{
    if ( currentPlaylistInterface() )
    {
        disconnect( currentPlaylistInterface()->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numTracksChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numShownChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                    this,                                 SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( shuffleModeChanged( bool ) ),
                    this,                                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
PlaylistManager::updateView()
{
    if ( currentPlaylistInterface() )
    {
        connect( currentPlaylistInterface()->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                                                       SIGNAL( numTracksChanged( unsigned int ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                                                       SIGNAL( numShownChanged( unsigned int ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                                                       SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( shuffleModeChanged( bool ) ),
                                                       SIGNAL( shuffleModeChanged( bool ) ) );

        m_topbar->setFilter( currentPlaylistInterface()->filter() );
    }

    if ( currentPage()->showStatsBar() && currentPlaylistInterface() )
    {
        emit numTracksChanged( currentPlaylistInterface()->unfilteredTrackCount() );

        if ( !currentPlaylistInterface()->filter().isEmpty() )
            emit numShownChanged( currentPlaylistInterface()->trackCount() );
        else
            emit numShownChanged( currentPlaylistInterface()->unfilteredTrackCount() );

        emit repeatModeChanged( currentPlaylistInterface()->repeatMode() );
        emit shuffleModeChanged( currentPlaylistInterface()->shuffled() );
        emit modeChanged( currentPlaylistInterface()->viewMode() );
    }

    if ( currentPage()->queueVisible() ) 
        m_queueView->show();
    else
        m_queueView->hide();

    emit statsAvailable( currentPage()->showStatsBar() );
    emit modesAvailable( currentPage()->showModes() );

    if ( !currentPage()->showStatsBar() && !currentPage()->showModes() )
        m_topbar->setVisible( false );
    else
        m_topbar->setVisible( true );

    m_infobar->setCaption( currentPage()->title() );
    m_infobar->setDescription( currentPage()->description() );
    m_infobar->setPixmap( currentPage()->pixmap() );
}


void
PlaylistManager::onWidgetDestroyed( QWidget* widget )
{
    qDebug() << "Destroyed child:" << widget;

    bool resetWidget = ( m_stack->currentWidget() == widget );
    m_stack->removeWidget( widget );

    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );
        if ( page->widget() == widget )
        {
            m_pageHistory.removeAt( i );
            if ( m_historyPosition > i )
                m_historyPosition--;
            break;
        }
    }

    if ( resetWidget )
    {
        if ( m_pageHistory.count() )
            showHistory( m_pageHistory.count() - 1 );
    }
}


void
PlaylistManager::setRepeatMode( PlaylistInterface::RepeatMode mode )
{
    if ( currentPlaylistInterface() )
        currentPlaylistInterface()->setRepeatMode( mode );
}


void
PlaylistManager::setShuffled( bool enabled )
{
    if ( currentPlaylistInterface() )
        currentPlaylistInterface()->setShuffled( enabled );
}


void 
PlaylistManager::createPlaylist( const Tomahawk::source_ptr& src,
                                 const QVariant& contents )
{
    Tomahawk::playlist_ptr p = Tomahawk::playlist_ptr( new Tomahawk::Playlist( src ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void 
PlaylistManager::createDynamicPlaylist( const Tomahawk::source_ptr& src,
                                        const QVariant& contents )
{
    Tomahawk::dynplaylist_ptr p = Tomahawk::dynplaylist_ptr( new Tomahawk::DynamicPlaylist( src, contents.toMap().value( "type", QString() ).toString()  ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


ViewPage*
PlaylistManager::pageForInterface( PlaylistInterface* interface ) const
{
    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );
        if ( page->playlistInterface() == interface )
            return page;
    }

    return 0;
}


int
PlaylistManager::positionInHistory( ViewPage* page ) const
{
    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        if ( page == m_pageHistory.at( i ) )
            return i;
    }

    return -1;
}


PlaylistInterface*
PlaylistManager::currentPlaylistInterface() const
{
    if ( currentPage() )
        return currentPage()->playlistInterface();
    else
        return 0;
}


Tomahawk::ViewPage*
PlaylistManager::currentPage() const
{
    if ( m_historyPosition >= 0 )
        return m_pageHistory.at( m_historyPosition );
    else
        return 0;
}


void
PlaylistManager::setHistoryPosition( int position )
{
    m_historyPosition = position;

    emit historyBackAvailable( m_historyPosition > 0 );
    emit historyForwardAvailable( m_historyPosition < m_pageHistory.count() - 1 );
}


Tomahawk::playlist_ptr
PlaylistManager::playlistForInterface( PlaylistInterface* interface ) const
{
    foreach ( PlaylistView* view, m_playlistViews.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_playlistViews.key( view );
        }
    }

    return playlist_ptr();
}


Tomahawk::dynplaylist_ptr
PlaylistManager::dynamicPlaylistForInterface( PlaylistInterface* interface ) const
{
    foreach ( DynamicWidget* view, m_dynamicWidgets.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_dynamicWidgets.key( view );
        }
    }

    return dynplaylist_ptr();
}


Tomahawk::collection_ptr
PlaylistManager::collectionForInterface( PlaylistInterface* interface ) const
{
    foreach ( CollectionView* view, m_collectionViews.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_collectionViews.key( view );
        }
    }
    foreach ( AlbumView* view, m_collectionAlbumViews.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_collectionAlbumViews.key( view );
        }
    }

    return collection_ptr();
}


bool
PlaylistManager::isSuperCollectionVisible() const
{
    return ( m_pageHistory.count() &&
           ( currentPage()->playlistInterface() == m_superCollectionView->playlistInterface() ||
             currentPage()->playlistInterface() == m_superAlbumView->playlistInterface() ) );
}


void
PlaylistManager::showCurrentTrack()
{
    ViewPage* page = pageForInterface( AudioEngine::instance()->currentTrackPlaylist() );
    setPage( page );
    page->jumpToCurrentTrack();
}


void
PlaylistManager::onPlayClicked()
{
    emit playClicked();
}


void
PlaylistManager::onPauseClicked()
{
    emit pauseClicked();
}

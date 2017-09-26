
#include "Mediator.hpp"
#include "Mediated.hpp"
#include "RoomBuilder.hpp"
#include "Item.hpp"
#include "GridItem.hpp"
#include "FreeItem.hpp"
#include "PlayerItem.hpp"
#include "FloorTile.hpp"
#include "Behavior.hpp"


namespace isomot
{

Mediator::Mediator( Room* room )
{
        this->room = room;
        this->transparencies = new TableOfTransparencies() ;
        this->gridItemsSorting = false;
        this->freeItemsSorting = false;
        this->switchInRoomIsOn = false;
        this->activePlayer = 0;

        pthread_mutex_init( &gridItemsMutex, 0 );
        pthread_mutex_init( &freeItemsMutex, 0 );

        // items which may be freezed by doughnut or switch
        badBoys.push_back( "behavior of detector" );
        badBoys.push_back( "behavior of hunter in four directions" );
        badBoys.push_back( "behavior of waiting hunter in four directions" );
        badBoys.push_back( "behavior of waiting hunter in eight directions" );
        badBoys.push_back( "behavior of there and back" );
        badBoys.push_back( "behavior of move then turn left and move" );
        badBoys.push_back( "behavior of move then turn right and move" );
        badBoys.push_back( "behavior of random patroling in four primary directions" );
        badBoys.push_back( "behavior of random patroling in four secondary directions" );
        badBoys.push_back( "behavior of random patroling in four secondary directions" );
        badBoys.push_back( "behavior of random patroling in eight directions" );

        // structure of room
        for ( int i = 0; i < room->getTilesX() * room->getTilesY() + 1; i++ )
        {
                structure.push_back( Column() );
        }
}

Mediator::~Mediator()
{
        // Elimina los elementos rejilla
        for ( size_t i = 0; i < structure.size(); i++ )
        {
                std::for_each( structure[ i ].begin(), structure[ i ].end (), DeleteObject() );
                structure[ i ].clear();
        }

        // Elimina los elementos libres
        std::for_each( freeItems.begin (), freeItems.end (), DeleteObject() );

        // Elimina los mútex
        pthread_mutex_destroy( &gridItemsMutex );
        pthread_mutex_destroy( &freeItemsMutex );

        delete transparencies;
}

void Mediator::update()
{
        // Acceso exclusivo a las listas de elementos rejilla
        pthread_mutex_lock( &gridItemsMutex );

        // Ordenación de las listas de elementos rejilla, si procede
        if ( this->gridItemsSorting )
        {
                for ( unsigned int i = 0; i < structure.size(); i++ )
                {
                        if ( ! structure[ i ].empty() )
                        {
                                structure[ i ].sort( sortGridItemList );
                        }
                }
                this->gridItemsSorting = false;
        }

        // Elementos libres que se eliminarán después de la actualización
        std::stack< GridItem * > gridItemsToDelete;

        // Actualiza los elementos rejilla
        for ( unsigned int i = 0; i < structure.size(); i++ )
        {
                for ( std::list< GridItem* >::iterator g = structure[ i ].begin (); g != structure[ i ].end (); ++g )
                {
                        if ( static_cast< GridItem* >( *g )->update() )
                                gridItemsToDelete.push( static_cast< GridItem * >( *g ) );
                }
        }

        // Se eliminan los elementos rejilla que pudieran haberse destruido
        while ( ! gridItemsToDelete.empty() )
        {
                GridItem* gi = gridItemsToDelete.top();
                room->removeGridItem( gi );
                gridItemsToDelete.pop();
        }

        // Libera el acceso exclusivo a las listas de elementos rejilla
        pthread_mutex_unlock( &gridItemsMutex );

        // Acceso exclusivo a la lista de elementos libres
        pthread_mutex_lock( &freeItemsMutex );

        // Ordenación de la lista de elementos libres, si procede
        if ( this->freeItemsSorting )
        {
                freeItems.sort( sortFreeItemList );
                this->freeItemsSorting = false;

                // Después de una reordenación es necesario volver a enmascarar los elementos porque
                // los solapamientos habrán cambiado
                for ( std::list< FreeItem * >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
                {
                        dynamic_cast< FreeItem * >( *f )->setWhichMask( WantMask );
                }
        }

        if ( activePlayer != 0 )
        {
                activePlayer->behave ();
        }

        // Elementos libres que se eliminarán después de la actualización
        std::stack < FreeItem * > freeItemsToDelete;

        // Actualiza los elementos libres
        for ( std::list< FreeItem * >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                if ( static_cast< FreeItem* >( *f )->update() )
                        freeItemsToDelete.push( static_cast< FreeItem * >( *f ) );
        }

        // Se eliminan los elementos libres que pudieran haberse destruido
        while ( ! freeItemsToDelete.empty() )
        {
                FreeItem* fi = freeItemsToDelete.top();
                room->removeFreeItem( fi );
                freeItemsToDelete.pop();
        }

        // Tras la actualización, se comprueba si algún jugador ha abandonado la sala o ha sido eliminado
        for ( std::vector< PlayerItem * >::iterator p = playerItems.begin (); p != playerItems.end (); ++p )
        {
                if ( ( *p )->getExit() != NoExit )
                {
                        // Debe suspenderse la actualización de la sala
                        room->setExit( ( *p )->getExit() );
                        room->setActive( false );
                }
        }

        // Libera el acceso exclusivo a la lista de elementos libres
        pthread_mutex_unlock( &freeItemsMutex );
}

void Mediator::beginUpdate()
{
        this->threadRunning = true;

        pthread_attr_t attr;

        pthread_attr_init( &attr );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );

        pthread_create( &thread, 0, updateThread, reinterpret_cast< void * >( this ) );

        pthread_attr_destroy( &attr );
}

void Mediator::endUpdate()
{
        this->threadRunning = false;
        pthread_join( thread, 0 );
}

void* updateThread( void* thisClass )
{
        Mediator* mediator = reinterpret_cast< Mediator* >( thisClass );

        while( mediator->isThreadRunning() )
        {
                mediator->update();
                sleep( 10 );
        }

        pthread_exit( 0 );
}

void Mediator::markItemsForMasking( FreeItem* item )
{
        if ( ! item ) return;

        // Se recorre la lista de elementos libres para ver cúales hay que volver a enmascarar
        for ( std::list< FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                FreeItem* thatFreeItem = *f;

                if ( thatFreeItem->getId() != item->getId() && thatFreeItem->getImage() )
                {
                        // El elemento debe enmascararse si hay solapamiento entre las imágenes
                        if ( ( thatFreeItem->getOffsetX() < item->getOffsetX() + item->getImage()->w )
                                && ( thatFreeItem->getOffsetX() + thatFreeItem->getImage()->w > item->getOffsetX() )
                                && ( thatFreeItem->getOffsetY() < item->getOffsetY() + item->getImage()->h )
                                && ( thatFreeItem->getOffsetY() + thatFreeItem->getImage()->h > item->getOffsetY() ) )
                        {
                                thatFreeItem->setWhichMask( WantMask );
                        }
                }
        }
}

void Mediator::markItemsForMasking( GridItem* gridItem )
{
        if ( ! gridItem ) return;

        // Se recorre la lista de elementos libres para ver cúales hay que volver a enmascarar
        for ( std::list< FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                FreeItem* freeItem = *f;

                if ( freeItem->getImage() )
                {
                        // El elemento debe enmascararse si hay solapamiento entre las imágenes
                        if ( ( freeItem->getOffsetX() < gridItem->getOffsetX() + gridItem->getImage()->w )
                                && ( freeItem->getOffsetX() + freeItem->getImage()->w > gridItem->getOffsetX() )
                                && ( freeItem->getOffsetY() < gridItem->getOffsetY() + gridItem->getImage()->h )
                                && ( freeItem->getOffsetY() + freeItem->getImage()->h > gridItem->getOffsetY() ) )
                        {
                                // Se comprueba si el elemento libre está espacialmente detrás del elemento rejilla
                                // tanto antes como después del cambio de posición
                                if ( ( freeItem->getX() < gridItem->getX() + gridItem->getWidthX() )
                                        && ( freeItem->getY() - freeItem->getWidthY() < gridItem->getY() )
                                        && ( freeItem->getZ() < gridItem->getZ() + gridItem->getHeight() ) )
                                {
                                        freeItem->setWhichMask( WantMask );
                                }
                        }
                }
        }
}

void Mediator::markItemsForShady( GridItem* item )
{
        // Marca para sombrear todos los elementos libres que tiene por debajo
        this->markFreeItemsForShady( static_cast< Item* >( item ) );

        // Marca para sombrear todos los elementos rejilla que tiene por debajo
        int column = room->tilesNumber.first * item->getCellY() + item->getCellX();

        if ( ! this->structure[column].empty() )
        {
                std::list< GridItem * >::iterator g = this->structure[ column ].begin ();
                GridItem* gridItem = *g;

                while( g != this->structure[ column ].end() && item->getId() != gridItem->getId() )
                {
                        gridItem->setWhichShade( WantShadow );
                        // Siguiente elemento rejilla de la columna
                        gridItem = *( ++g ) ;
                }
        }

        // Marca para sombrear la loseta de la columna, si existe
        if ( room->floor[ column ] != 0 )
                room->floor[ column ]->setWhichShade( WantShadow );
}

void Mediator::markItemsForShady( FreeItem* item )
{
        // Marca para sombrear todos los elementos libres que tiene por debajo
        this->markFreeItemsForShady( static_cast< Item* >( item ) );

        // Rango de columnas interseccionadas por el elemento
        int xStart = item->getX() / room->tileSize;
        int xEnd = ( item->getX() + item->getWidthX() - 1 ) / room->tileSize + 1;
        int yStart = ( item->getY() - item->getWidthY() + 1 ) / room->tileSize;
        int yEnd = item->getY() / room->tileSize + 1;

        // Para toda columna que intersecciona con el elemento:
        for ( int i = xStart; i < xEnd; i++ )
        {
                for ( int j = yStart; j < yEnd; j++ )
                {
                        // Todo elemento de la columna que se encuentre por debajo, se marca para sombrear
                        int column = room->getTilesX() * j + i;
                        for ( std::list< GridItem* >::iterator g = structure[ column ]. begin (); g != structure[ column ]. end (); ++g )
                        {
                                GridItem* gridItem = static_cast< GridItem* >( *g );

                                if ( item->getZ () > gridItem->getZ () )
                                {
                                        gridItem->setWhichShade( WantShadow );
                                }
                        }

                        // Marca para sombrear la loseta de la columna
                        if ( room->floor[ column ] != 0 )
                                room->floor[ column ]->setWhichShade( WantShadow );
                }
        }
}

void Mediator::markFreeItemsForShady( Item* item )
{
        // Se recorre la lista de elementos libres para ver cúales hay que sombrear
        for ( std::list < FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                FreeItem* freeItem = static_cast< FreeItem* >( *f );

                if ( freeItem->getId () != item->getId () )
                {
                        if ( ( freeItem->getX() + freeItem->getWidthX() > item->getX() ) && ( freeItem->getX() < item->getX() + item->getWidthX() )
                                && ( freeItem->getY() > freeItem->getY() - item->getWidthY() ) && ( freeItem->getY() - freeItem->getWidthY() < item->getY() )
                                && ( freeItem->getZ() < item->getZ() ) )
                        {
                                freeItem->setWhichShade( WantShadow );
                        }
                }
        }
}

void Mediator::castShadowOnFloor( FloorTile* floorTile )
{
        int xCell = floorTile->getX();
        int yCell = floorTile->getY();
        int column = floorTile->getColumn();
        int tileSize = room->getSizeOfOneTile();

        // Se sombrea con todos los elementos rejilla que pueda tener por encima
        for ( std::list< GridItem * >::iterator g = structure[ column ].begin (); g != structure[ column ].end (); ++g )
        {
                GridItem* gridItem = static_cast< GridItem * >( *g );

                // Si el elemento tiene sombra entonces se sombrea la imagen de la loseta
                if(  gridItem->getShadow() )
                {
                        floorTile->castShadowImage (
                                /* x */ ( tileSize << 1 ) * ( xCell - yCell ) - ( gridItem->getShadow()->w >> 1 ) + room->getX0() + 1,
                                /* y */ tileSize * ( xCell + yCell + 1 ) - ( gridItem->getShadow()->h >> 1 ) + room->getY0() - 1,
                                /* shadow */ gridItem->getShadow(),
                                /* shadingScale */ room->shadingScale
                                /* transparency = 0 (default) */
                        ) ;
                }
        }

        // Se sombrea con los elementos libres que pueda tener por encima
        for ( int percent = 0; percent <= 100; percent++ )
        {
                int howManyItems = transparencies->countItemsWithDegreeOfTransparency( percent );

                // Tiene que haber elementos en el porcentaje de transparencia actual
                if ( howManyItems != 0 )
                {
                        // Para todo elemento con ese grado de transparencia
                        for ( int n = 0; n < howManyItems; n++ )
                        {
                                // Recorre la lista de elementos libres para sombrear la loseta
                                for ( std::list< FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
                                {
                                        FreeItem* freeItem = static_cast< FreeItem* >( *f );

                                        if ( freeItem->getShadow() && freeItem->getTransparency() == percent )
                                        {
                                                // Rango de columnas que interseccionan por el elemento
                                                int xStart = freeItem->getX() / tileSize;
                                                int xEnd = ( freeItem->getX() + freeItem->getWidthX() - 1 ) / tileSize;
                                                int yStart = ( freeItem->getY() - freeItem->getWidthY() + 1 ) / tileSize;
                                                int yEnd = freeItem->getY() / tileSize;

                                                if ( xCell >= xStart && xCell <= xEnd && yCell >= yStart && yCell <= yEnd )
                                                {
                                                        floorTile->castShadowImage (
                                                                /* x */ ( ( freeItem->getX() - freeItem->getY() ) << 1 ) + room->getX0() + ( freeItem->getWidthX() + freeItem->getWidthY() ) - ( ( freeItem->getShadow()->w ) >> 1 ) - 1,
                                                                /* y */ freeItem->getX() + freeItem->getY() + room->getY0() + ( ( freeItem->getWidthX() - freeItem->getWidthY() + 1 ) >> 1 ) - ( ( freeItem->getShadow()->h ) >> 1 ),
                                                                /* shadow */ freeItem->getShadow(),
                                                                /* shadingScale */ room->shadingScale,
                                                                /* transparency */ freeItem->getTransparency()
                                                        ) ;
                                                }
                                        }
                                }
                        }
                }
        }
}

void Mediator::castShadowOnGrid( GridItem* gridItem )
{
        int tileSize = room->getSizeOfOneTile();
        int column = gridItem->getColumn();

        // shade with grid items it may have above
        for ( std::list< GridItem* >::iterator g = structure[ column ].begin(); g != structure[ column ].end(); ++g )
        {
                GridItem* tempItem = static_cast< GridItem* >( *g );

                if ( tempItem->getShadow() && tempItem->getZ() > gridItem->getZ() )
                {
                        gridItem->castShadowImage (
                                /* x */ gridItem->getOffsetX() + ( ( gridItem->getImage()->w - tempItem->getShadow()->w) >> 1 ),
                                /* y */ gridItem->getOffsetY() + gridItem->getImage()->h - room->getSizeOfOneTile() - gridItem->getHeight() - ( tempItem->getShadow()->h >> 1 ),
                                /* shadow */ tempItem->getShadow(),
                                /* shadingScale */ room->shadingScale
                                /* transparency = 0 (default) */
                        ) ;
                }
        }

        // shade with free items it may have above
        for ( int percent = 0; percent <= 100; percent++ )
        {
                int howManyItems = transparencies->countItemsWithDegreeOfTransparency( percent );

                // there are items with current amount of transparency
                if ( howManyItems != 0 )
                {
                        for ( int n = 0; n < howManyItems; n++ )
                        {
                                // scroll thru list of free items to shade this item
                                for ( std::list< FreeItem* >::iterator f = freeItems.begin(); f != freeItems.end(); ++f )
                                {
                                        FreeItem* freeItem = static_cast< FreeItem* >( *f );

                                        if ( freeItem->getShadow() && freeItem->getTransparency() == percent && freeItem->getZ() > gridItem->getZ() )
                                        {
                                                // Rango de columnas que interseccionan por el elemento
                                                int xStart = freeItem->getX() / tileSize;
                                                int xEnd = (freeItem->getX() + freeItem->getWidthX() - 1) / tileSize;
                                                int yStart = (freeItem->getY() - freeItem->getWidthY() + 1) / tileSize;
                                                int yEnd = freeItem->getY() / tileSize;

                                                if ( gridItem->getCellX() >= xStart && gridItem->getCellX() <= xEnd && gridItem->getCellY() >= yStart && gridItem->getCellY() <= yEnd )
                                                {
                                                        gridItem->castShadowImage (
                                                                /* x */ ( ( freeItem->getX() - freeItem->getY() ) << 1 ) + ( freeItem->getWidthX() + freeItem->getWidthY() ) - ( ( freeItem->getShadow()->w ) >> 1 ) - 1,
                                                                /* y */ freeItem->getX() + freeItem->getY() + ( ( freeItem->getWidthX() - freeItem->getWidthY() + 1 ) >> 1 ) - ( ( freeItem->getShadow()->h ) >> 1 ) - gridItem->getZ() - gridItem->getHeight(),
                                                                /* shadow */ freeItem->getShadow(),
                                                                /* shadingScale */ room->shadingScale,
                                                                /* transparency */ freeItem->getTransparency()
                                                        ) ;
                                                }
                                        }
                                }
                        }
                }
        }
}

void Mediator::castShadowOnFreeItem( FreeItem* freeItem )
{
        int tileSize = room->getSizeOfOneTile();

        // Rango de columnas que interseccionan con el elemento
        int xStart = freeItem->getX() / tileSize;
        int xEnd = ( freeItem->getX() + freeItem->getWidthX() - 1 ) / tileSize;
        int yStart = ( freeItem->getY() - freeItem->getWidthY() + 1 ) / tileSize;
        int yEnd = freeItem->getY() / tileSize;

        // shadow from grid items
        for ( int yCell = yStart; yCell <= yEnd; yCell++ )
        {
                for ( int xCell = xStart; xCell <= xEnd; xCell++ )
                {
                        int column = yCell * room->getTilesX() + xCell;

                        for ( std::list< GridItem* >::iterator g = structure[ column ].begin (); g != structure[ column ].end (); ++g )
                        {
                                GridItem* gridItem = static_cast< GridItem* >( *g );

                                if ( gridItem->getShadow() && gridItem->getZ() > freeItem->getZ() )
                                {
                                        freeItem->castShadowImage (
                                                /* x */ ( tileSize << 1 ) * ( xCell - yCell ) - ( gridItem->getShadow()->w >> 1 ) + 1,
                                                /* y */ tileSize * ( xCell + yCell + 1 ) - freeItem->getZ() - freeItem->getHeight() - ( gridItem->getShadow()->h >> 1 ) - 1,
                                                /* shadow */ gridItem->getShadow(),
                                                /* shadingScale */ room->shadingScale
                                                /* transparency = 0 (default) */
                                        ) ;
                                }
                        }
                }
        }

        // shadow from free items it may have above
        for ( int percent = 0; percent <= 100; percent++ )
        {
                int countOfItems = transparencies->countItemsWithDegreeOfTransparency( percent );

                // Tiene que haber elementos en el porcentaje de transparencia actual
                if ( countOfItems != 0 )
                {
                        // Para todo elemento con ese grado de transparencia
                        for ( int n = 0; n < countOfItems; n++ )
                        {
                                // Recorre la lista de elementos libres para sombrear el elemento
                                for ( std::list< FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
                                {
                                        FreeItem* tempItem = static_cast< FreeItem* >( *f );

                                        if ( tempItem->getShadow() && tempItem->getTransparency() == percent && tempItem->getId() != freeItem->getId() )
                                        {
                                                if ( freeItem->getZ() < tempItem->getZ() &&
                                                        freeItem->getX() < tempItem->getX() + tempItem->getWidthX() && tempItem->getX() < freeItem->getX() + freeItem->getWidthX() &&
                                                        freeItem->getY() > tempItem->getY() - tempItem->getWidthY() && tempItem->getY() > freeItem->getY() - freeItem->getWidthY() )
                                                {
                                                        freeItem->castShadowImage(
                                                                /* x */ ( ( tempItem->getX() - tempItem->getY() ) << 1 ) + tempItem->getWidthX() + tempItem->getWidthY() - ( tempItem->getShadow()->w >> 1 ) - 1,
                                                                /* y */ tempItem->getX() + tempItem->getY() - freeItem->getZ() - freeItem->getHeight() + ( ( tempItem->getWidthX() - tempItem->getWidthY() - tempItem->getShadow()->h ) >> 1 ),
                                                                /* shadow */ tempItem->getShadow(),
                                                                /* shadingScale */ room->shadingScale,
                                                                /* transparency */ freeItem->getTransparency()
                                                        );
                                                }
                                        }
                                }
                        }
                }
        }
}

void Mediator::mask( FreeItem* freeItem )
{
        // Se busca el elemento en la lista para comprobar las ocultaciones únicamente
        // con los elementos siguientes. Si la ordenación de la lista está bien hecha
        // no debe haber ocultaciones con los elementos anteriores
        std::list< FreeItem * >::iterator f = std::find_if( freeItems.begin (), freeItems.end (), std::bind2nd( EqualItemId(), freeItem->getId() ) );

        // Si se ha encontrado el elemento se procede a enmascarar
        if ( f != freeItems.end () )
        {
                // Se compara freeItem con los siguientes elementos libres de la lista para comprobar las ocultaciones
                while ( ++f != freeItems.end () )
                {
                        FreeItem* tempItem = static_cast< FreeItem * >( *f );

                        // El otro elemento tiene que tener imagen y uno de los dos tiene que estar
                        // marcado para enmascararlo; además el otro no tiene que ser transparente
                        if ( tempItem->getImage() &&
                                ( ( freeItem->whichMask() != NoMask && tempItem->getTransparency() == 0 ) ||
                                ( tempItem->whichMask() != NoMask && freeItem->getTransparency() == 0 ) ) )
                        {
                                // Se comprueba si sus gráficos se solapan
                                if ( ( tempItem->getOffsetX() < freeItem->getOffsetX() + freeItem->getImage()->w ) &&
                                        ( tempItem->getOffsetX() + tempItem->getImage()->w > freeItem->getOffsetX() ) &&
                                        ( tempItem->getOffsetY() < freeItem->getOffsetY() + freeItem->getImage()->h ) &&
                                        ( tempItem->getOffsetY() + tempItem->getImage()->h > freeItem->getOffsetY() ) )
                                {
                                        // freeItem está detrás de tempItem
                                        if ( ( freeItem->getX() + freeItem->getWidthX() <= tempItem->getX() ) ||
                                                ( freeItem->getY() <= tempItem->getY() - tempItem->getWidthY() ) ||
                                                ( freeItem->getZ() + freeItem->getHeight() <= tempItem->getZ() ) )
                                        {
                                                freeItem->maskImage( tempItem->getOffsetX(), tempItem->getOffsetY(), tempItem->getImage() );
                                        }
                                        // tempItem está detrás de freeItem
                                        else
                                        {
                                                tempItem->maskImage( freeItem->getOffsetX(), freeItem->getOffsetY(), freeItem->getImage() );
                                        }
                                }
                        }
                }
        }

        // Se compara freeItem con los elementos rejilla que tiene debajo para comprobar las ocultaciones
        int xStart = freeItem->getX() / room->getSizeOfOneTile();
        int xEnd = ( ( freeItem->getX() + freeItem->getWidthX() - 1 ) / room->getSizeOfOneTile() ) + 1;
        int yStart = ( freeItem->getY() / room->getSizeOfOneTile() ) + 1;
        int yEnd = ( freeItem->getY() - freeItem->getWidthY() + 1 ) / room ->getSizeOfOneTile();

        // Si el elemento libre hay que enmascararlo, se procede a hacerlo con los elementos rejilla
        if ( freeItem->whichMask() != NoMask )
        {
                do
                {
                        int i = 0;

                        while ( ( xStart + i < room->getTilesX() ) && ( yStart + i < room->getTilesY() ) )
                        {
                                int column = room->getTilesX() * ( yStart + i ) + xStart + i;

                                for ( std::list< GridItem * >::iterator g = structure[ column ].begin (); g != structure[ column ].end (); ++g )
                                {
                                        GridItem* gridItem = static_cast< GridItem * >( *g );

                                        // El elemento rejilla tiene que tener una imagen
                                        if ( gridItem->getImage() )
                                        {
                                                // Se comprueba si sus gráficos se solapan
                                                if ( ( gridItem->getOffsetX() < freeItem->getOffsetX() + freeItem->getImage()->w ) &&
                                                        ( gridItem->getOffsetX() + gridItem->getImage()->w > freeItem->getOffsetX() ) &&
                                                        ( gridItem->getOffsetY() < freeItem->getOffsetY() + freeItem->getImage()->h ) &&
                                                        ( gridItem->getOffsetY() + gridItem->getImage()->h > freeItem->getOffsetY() ) )
                                                {
                                                        // freeItem está detrás de gridItem
                                                        if ( ( freeItem->getX() + freeItem->getWidthX() <= ( xStart + i ) * room->getSizeOfOneTile() ) ||
                                                                ( freeItem->getY() <= (yStart + i + 1) * room->getSizeOfOneTile() - 1 - room->getSizeOfOneTile() ) ||
                                                                ( freeItem->getZ() + freeItem->getHeight() <= gridItem->getZ() ) )
                                                        {
                                                                freeItem->maskImage( gridItem->getOffsetX(), gridItem->getOffsetY(), gridItem->getImage() );
                                                        }
                                                }
                                        }
                                }
                                i++;
                        }

                        if ( yStart != yEnd )
                        {
                                yStart--;
                        }
                        else
                        {
                                xStart++;
                        }
                }
                while ( xStart <= xEnd );
        }
}

Item* Mediator::findItemById( int id )
{
        Item* item = 0;

        // Búsqueda en la lista de elementos libres
        if ( id & 1 )
        {
                std::list< FreeItem * >::iterator f = std::find_if( freeItems.begin (), freeItems.end (), std::bind2nd( EqualItemId (), id ) );

                if ( f != freeItems.end () )
                {
                        item = dynamic_cast< Item * >( *f );
                }
        }
        // Búsqueda en las listas de elementos rejilla
        else
        {
                std::list< GridItem * >::iterator g;

                for ( int i = 0; i < room->getTilesX() * room->getTilesY(); i++ )
                {
                        g = std::find_if( structure[ i ].begin (), structure[ i ].end (), std::bind2nd( EqualItemId (), id ) );

                        if ( g != structure[ i ].end() )
                        {
                                item = dynamic_cast< Item * >( *g );
                                i = room->getTilesX() * room->getTilesY();
                        }
                }
        }

        return item;
}

Item* Mediator::findItemByLabel( const std::string& label )
{
        Item* item = 0;

        // search in free items
        std::list< FreeItem * >::iterator f = std::find_if( freeItems.begin (), freeItems.end (), std::bind2nd( EqualItemLabel (), label ) );

        if ( f != freeItems.end () )
        {
                item = dynamic_cast< Item * >( *f );
        }

        // search in grid items
        if ( item == 0 )
        {
                std::list< GridItem * >::iterator g;

                for ( int i = 0; i < room->getTilesX() * room->getTilesY(); i++ )
                {
                        g = std::find_if( structure[ i ].begin (), structure[ i ].end (), std::bind2nd( EqualItemLabel (), label ) );

                        if ( g != structure[ i ].end () )
                        {
                                item = dynamic_cast< Item * >( *g );
                                i = room->getTilesX() * room->getTilesY();
                        }
                }
        }

        return item;
}

Item* Mediator::findItemByBehavior( const std::string& behavior )
{
        Item* item = 0;

        // search in free items
        std::list< FreeItem * >::iterator f = std::find_if( freeItems.begin (), freeItems.end (), std::bind2nd( EqualBehaviorOfItem (), behavior ) );

        if ( f != freeItems.end () )
        {
                item = dynamic_cast< Item * >( *f );
        }

        // search in grid items
        if ( item == 0 )
        {
                std::list< GridItem * >::iterator g;

                for ( int i = 0; i < room->getTilesX() * room->getTilesY(); i++ )
                {
                        g = std::find_if( structure[ i ].begin (), structure[ i ].end (), std::bind2nd( EqualBehaviorOfItem (), behavior ) );

                        if ( g != structure[ i ].end () )
                        {
                                item = dynamic_cast< Item * >( *g );
                                i = room->getTilesX() * room->getTilesY();
                        }
                }
        }

        return item;
}

bool Mediator::findCollisionWithItem( Item * item )
{
        bool collisionFound = false;

        // for grid item or free item with ability to detect collisions
        if ( dynamic_cast< GridItem * >( item ) || ( dynamic_cast< FreeItem * >( item ) && dynamic_cast< FreeItem * >( item )->isCollisionDetector() ) )
        {
                // traverse list of free items looking for collisions
                for ( std::list< FreeItem * >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
                {
                        FreeItem* freeItem = static_cast< FreeItem * >( *f );

                        if ( freeItem->getId() != item->getId() && freeItem->isCollisionDetector() )
                        {
                                // look for intersection
                                if ( ( freeItem->getX() + freeItem->getWidthX() > item->getX() ) && ( freeItem->getX() < item->getX() + item->getWidthX() ) &&
                                        ( freeItem->getY() > item->getY() - item->getWidthX() ) && ( freeItem->getY() - freeItem->getWidthY() < item->getY() ) &&
                                        ( freeItem->getZ() + freeItem->getHeight() > item->getZ() ) && ( freeItem->getZ() < item->getZ() + item->getHeight() ) )
                                {
                                        collisions.push_back( freeItem->getId() );
                                        collisionFound = true;
                                }
                        }
                }

                // for grid item
                if ( dynamic_cast< GridItem * >( item ) )
                {
                        GridItem* temp = dynamic_cast< GridItem * >( item );
                        int column = room->getTilesX() * temp->getCellY() + temp->getCellX();

                        // scroll through lists of grid items looking for collisions
                        for ( std::list< GridItem * >::iterator g = this->structure[ column ].begin (); g != this->structure[ column ].end (); ++g )
                        {
                                GridItem* gridItem = static_cast< GridItem * >( *g );

                                if ( gridItem->getId() != item->getId() )
                                {
                                        // look for intersection
                                        if ( ( gridItem->getX() + gridItem->getWidthX() > item->getX() ) && ( gridItem->getX() < item->getX() + item->getWidthX() ) &&
                                                ( gridItem->getY() > item->getY() - item->getWidthX() ) && ( gridItem->getY() - gridItem->getWidthY() < item->getY() ) &&
                                                ( gridItem->getZ() + gridItem->getHeight() > item->getZ() ) && ( gridItem->getZ() < item->getZ() + item->getHeight() ) )
                                        {
                                                collisions.push_back( gridItem->getId() );
                                                collisionFound = true;
                                        }
                                }
                        }
                }
                // for free item
                else if( dynamic_cast< FreeItem * >( item ) )
                {
                        int xStart = item->getX() / room->tileSize;
                        int xEnd = ( item->getX() + item->getWidthX() - 1 ) / room->tileSize + 1;
                        int yStart = ( item->getY() - item->getWidthY() + 1 ) / room->tileSize;
                        int yEnd = item->getY() / room->tileSize + 1;

                        // in case when item collides with some wall
                        if ( xStart < 0 ) xStart = 0;
                        if ( xEnd > room->getTilesX() ) xEnd = room->getTilesX();
                        if ( yStart < 0 ) yStart = 0;
                        if ( yEnd > room->getTilesY() ) yEnd = room->getTilesY();

                        // see collision only in range of columns intersecting with item
                        for ( int i = xStart; i < xEnd; i++ )
                        {
                                for ( int j = yStart; j < yEnd; j++ )
                                {
                                        for ( std::list< GridItem * >::iterator g = this->structure[ room->getTilesX() * j + i ].begin ();
                                                g != this->structure[ room->getTilesX() * j + i ].end (); ++g )
                                        {
                                                GridItem* gridItem = static_cast< GridItem * >( *g );

                                                if ( ( gridItem->getZ() + gridItem->getHeight() > item->getZ() ) && ( gridItem->getZ() < item->getZ() + item->getHeight() ) )
                                                {
                                                        collisions.push_back( gridItem->getId() );
                                                        collisionFound = true;
                                                }
                                        }
                                }
                        }
                }
        }

        return collisionFound;
}

int Mediator::findHighestZ( Item * item )
{
        int z( 0 );

        // Se recorre la lista de elementos libres buscando aquel que intersecta
        // con la columna y de mayor altura
        for ( std::list< FreeItem * >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                FreeItem* freeItem = static_cast< FreeItem * >( *f );

                if ( freeItem->getId() != item->getId() && freeItem->isCollisionDetector() )
                {
                        // Se busca la intersección en los ejes X e Y y la mayor Z
                        if ( ( freeItem->getX() + freeItem->getWidthX() > item->getX() ) && ( freeItem->getX() < item->getX() + item->getWidthX() ) &&
                                ( freeItem->getY() > item->getY() - item->getWidthX() ) && ( freeItem->getY() - freeItem->getWidthY() < item->getY() ) &&
                                ( freeItem->getZ() + freeItem->getHeight() > item->getZ() ) )
                        {
                                z = freeItem->getZ() + freeItem->getHeight();
                        }
                }
        }

        // Búsqueda en la estructura de la sala en el caso de que sea un elemento rejilla
        if ( dynamic_cast< GridItem * >( item ) )
        {
                GridItem* gridItem = dynamic_cast< GridItem * >( item );
                int column( room->getTilesX() * gridItem->getCellY() + gridItem->getCellX() );

                // Se recorren los elementos de la columna buscando el mayor
                for ( std::list< GridItem * >::iterator g = this->structure[ column ].begin (); g != this->structure[ column ].end (); ++g )
                {
                        GridItem* tempItem = static_cast< GridItem * >( *g );

                        if ( tempItem->getZ() + tempItem->getHeight() > z )
                        {
                                z = tempItem->getZ() + tempItem->getHeight();
                        }
                }
        }
        // Búsqueda en la estructura de la sala en el caso de que sea un elemento libre
        else if ( dynamic_cast< FreeItem * >( item ) )
        {
                FreeItem* freeItem = dynamic_cast< FreeItem * >( item );

                // Rango de columnas intersectadas por el elemento
                int xStart = freeItem->getX() / room->getSizeOfOneTile();
                int xEnd = ( freeItem->getX() + freeItem->getWidthX() - 1 ) / room->getSizeOfOneTile() + 1;
                int yStart = ( freeItem->getY() - freeItem->getWidthY() + 1 ) / room->getSizeOfOneTile();
                int yEnd = freeItem->getY() / room->getSizeOfOneTile() + 1;

                // Se recorren los elementos de la columas intersectadas
                for ( int i = xStart; i < xEnd; i++ )
                {
                        for ( int j = yStart; j < yEnd; j++ )
                        {
                                int column( room->getTilesX() * j + i );
                                for ( std::list< GridItem * >::iterator g = structure[ column ].begin (); g != structure[ column ].end (); ++g )
                                {
                                        GridItem* gridItem = static_cast< GridItem * >( *g );

                                        if ( gridItem->getZ() + gridItem->getHeight() > z )
                                        {
                                                z = gridItem->getZ() + gridItem->getHeight();
                                        }
                                }
                        }
                }
        }

        return z;
}

void Mediator::addItem( GridItem* gridItem )
{
        // Si es un elemento rejilla, se inserta al final de la lista de su columna correspondiente
        // y luego se ordena dicha lista
        pthread_mutex_lock( &gridItemsMutex );

        int column( room->getTilesX() * gridItem->getCellY() + gridItem->getCellX() );

        structure[ column ].push_back( gridItem );
        structure[ column ].sort( sortGridItemList );

        pthread_mutex_unlock( &gridItemsMutex );
}

void Mediator::addItem( FreeItem* freeItem )
{
        // Si es un elemento libre, se inserta al final de la lista y luego se realiza la ordenación
        freeItems.push_back( freeItem );
        freeItems.sort( sortFreeItemList );
}

void Mediator::addItem( PlayerItem* playerItem )
{
        // Si es un jugador, se inserta al final de la lista de elementos libres porque para dibujarlo
        // se trata como si fuera un elemento libre y luego se realiza la ordenación
        freeItems.push_back( playerItem );
        freeItems.sort( sortFreeItemList );
        // Además se inserta en la lista de jugadores
        playerItems.push_back( playerItem );
}

void Mediator::removeItem( GridItem* gridItem )
{
        // Se elimina de la lista que corresponde con su columna
        int column( room->getTilesX() * gridItem->getCellY() + gridItem->getCellX() );
        structure[ column ].erase(
                std::remove_if(
                        structure[ column ].begin(),
                        structure[ column ].end (),
                        std::bind2nd( EqualItemId(), gridItem->getId() ) ),
                structure[ column ].end () );
}

void Mediator::removeItem( FreeItem* freeItem )
{
        // Se elimina de la lista única de elementos libres
        freeItems.erase(
                std::remove_if(
                        freeItems.begin (), freeItems.end (),
                        std::bind2nd( EqualItemId(), freeItem->getId() ) ),
                freeItems.end () );
}

void Mediator::removeItem( PlayerItem* playerItem )
{
        // Los jugadores se almacenan en la lista de los elementos libres, porque también lo son
        this->removeItem( static_cast< FreeItem* >( playerItem ) );

        // Además, se elimina de la lista de jugadores
        std::vector< PlayerItem* >::iterator i = playerItems.erase(
                std::remove_if(
                        playerItems.begin(), playerItems.end(),
                        std::bind2nd( EqualPlayerItemId(), playerItem->getId() ) ),
                playerItems.end() );
        activePlayer = ( i != playerItems.end () ? *i : *playerItems.begin () );
}

void Mediator::pushCollision( int id )
{
        collisions.push_back( id );
}

int Mediator::popCollision()
{
        int id = 0;

        if ( ! collisions.empty() )
        {
                std::vector< int >::iterator i = collisions.begin ();
                id = *i;
                i = collisions.erase( i );
        }

        return id;
}

Item* Mediator::collisionWithByLabel( const std::string& label )
{
        for ( unsigned int i = 0; i < collisions.size(); i++ )
        {
                Item* item = findItemById( collisions[ i ] );

                if ( item != 0 && item->getLabel() == label )
                        return item;
        }

        return 0;
}

Item* Mediator::collisionWithByBehavior( const std::string& behavior )
{
        for ( unsigned int i = 0; i < collisions.size(); i++ )
        {
                Item* item = findItemById( collisions[ i ] );

                if ( item != 0 && item->getBehavior() != 0 && item->getBehavior()->getNameOfBehavior () == behavior )
                        return item;
        }

        return 0;
}

Item* Mediator::collisionWithBadBoy()
{
        for ( unsigned int i = 0; i < collisions.size(); i++ )
        {
                Item* item = findItemById( collisions[ i ] );

                if ( item != 0 && item->getBehavior() != 0 && item->isMortal()
                        && std::find( badBoys.begin(), badBoys.end(), item->getBehavior()->getNameOfBehavior () ) != badBoys.end() )
                {
                        return item;
                }
        }

        return 0;
}

bool Mediator::selectNextPlayer( ItemDataManager* itemDataManager )
{
        // get exclusive access to the list of free elements
        pthread_mutex_lock( &freeItemsMutex );

        PlayerItem* previousPlayer = activePlayer;

        // search for next player
        std::vector< PlayerItem * >::iterator i = std::find_if(
                playerItems.begin (), playerItems.end (),
                std::bind2nd( EqualPlayerItemId(), activePlayer->getId() ) );
        ++i;
        activePlayer = ( i != playerItems.end () ? ( *i ) : *playerItems.begin () );

        // see if players may join
        if ( previousPlayer != activePlayer )
        {
                const int delta = room->getSizeOfOneTile() >> 1;

                if ( ( previousPlayer->getLabel() == "head" && activePlayer->getLabel() == "heels" ) ||
                                ( previousPlayer->getLabel() == "heels" && activePlayer->getLabel() == "head" ) )
                {
                        if ( ( previousPlayer->getX() + delta >= activePlayer->getX() )
                                && ( previousPlayer->getX() + previousPlayer->getWidthX() - delta <= activePlayer->getX() + activePlayer->getWidthX() )
                                && ( previousPlayer->getY() + delta >= activePlayer->getY() )
                                && ( previousPlayer->getY() + previousPlayer->getWidthY() - delta <= activePlayer->getY() + activePlayer->getWidthY() )
                                && ( ( previousPlayer->getLabel() == "head" && previousPlayer->getZ() - LayerHeight == activePlayer->getZ() ) ||
                                        ( previousPlayer->getLabel() == "heels" && activePlayer->getZ() - LayerHeight == previousPlayer->getZ() ) ) )
                        {
                                PlayerItem* reference = previousPlayer->getLabel() == "heels" ? previousPlayer : activePlayer;
                                this->lastActivePlayer = previousPlayer->getLabel() == "heels" ? "heels" : "head";
                                int x = reference->getX();
                                int y = reference->getY();
                                int z = reference->getZ();
                                Direction direction = reference->getDirection();

                                // item that Heels may hold in handbag
                                ItemData* takenItemData = 0;
                                BITMAP* takenItemImage = 0;
                                std::string behaviorOfItemTaken = "still";
                                if ( previousPlayer->getLabel() == "heels" )
                                {
                                        behaviorOfItemTaken = previousPlayer->consultTakenItem( takenItemData );
                                        takenItemImage = previousPlayer->consultTakenItemImage();
                                }
                                else
                                {
                                        behaviorOfItemTaken = activePlayer->consultTakenItem( takenItemData );
                                        takenItemImage = activePlayer->consultTakenItemImage();
                                }

                                // remove "simple" players
                                this->room->removePlayer( previousPlayer );
                                this->room->removePlayer( activePlayer );

                                // create "composite" player
                                std::auto_ptr< RoomBuilder > roomBuilder( new RoomBuilder( itemDataManager ) );
                                activePlayer = roomBuilder->createPlayerInRoom( this->room, "headoverheels", "behavior of Head over Heels", x, y, z, direction, false );
                                // transfer item in handbag
                                activePlayer->assignTakenItem( takenItemData, takenItemImage, behaviorOfItemTaken );

                                // release exclusive access to the list of free items
                                pthread_mutex_unlock( &freeItemsMutex );

                                return true;
                        }
                }
        }
        // if active player is the same as previous player then it is composite player
        else if ( activePlayer->getLabel() == "headoverheels" )
        {
                int x = activePlayer->getX();
                int y = activePlayer->getY();
                int z = activePlayer->getZ();
                Direction direction = activePlayer->getDirection();

                // get data of item in handbag
                ItemData* takenItemData = 0;
                BITMAP* takenItemImage = 0;
                std::string behaviorOfItemTaken = "still";
                behaviorOfItemTaken = activePlayer->consultTakenItem( takenItemData );
                takenItemImage = activePlayer->consultTakenItemImage();

                // remove "composite" player
                this->room->removePlayer( activePlayer );

                // create "simple" players
                std::auto_ptr< RoomBuilder > roomBuilder( new RoomBuilder( itemDataManager ) );
                previousPlayer = roomBuilder->createPlayerInRoom( this->room, "heels", "behavior of Heels", x, y, z, direction, false );
                previousPlayer->assignTakenItem( takenItemData, takenItemImage, behaviorOfItemTaken );
                activePlayer = roomBuilder->createPlayerInRoom( this->room, "head", "behavior of Head", x, y, z + LayerHeight, direction, false );

                if ( this->lastActivePlayer == "head" )
                {
                        std::swap( previousPlayer, activePlayer );
                }
        }

        // release exclusive access to the list of free items
        pthread_mutex_unlock( &freeItemsMutex );

        return ( previousPlayer != activePlayer );
}

bool Mediator::selectAlivePlayer( ItemDataManager* itemDataManager )
{
        PlayerItem* previousPlayer = activePlayer;

        std::vector< PlayerItem* >::iterator i = std::find_if( playerItems.begin (), playerItems.end (), std::bind2nd( EqualPlayerItemId(), activePlayer->getId() ) );
        ++i;
        activePlayer = ( i != playerItems.end () ? ( *i ) : *playerItems.begin () );

        if ( previousPlayer != activePlayer )
        {
                this->removeItem( previousPlayer );
                previousPlayer = 0;
        }

        return ( previousPlayer != activePlayer );
}

void Mediator::toggleSwitchInRoom ()
{
        this->switchInRoomIsOn = ! this->switchInRoomIsOn ;

        // Búsqueda en la lista de elementos libres de elementos volátiles y mortales móviles para congelarlos
        for ( std::list< FreeItem* >::iterator f = freeItems.begin (); f != freeItems.end (); ++f )
        {
                FreeItem* freeItem = *f;

                if ( freeItem != 0 && freeItem->getBehavior() != 0 )
                {
                        std::string behavior = freeItem->getBehavior()->getNameOfBehavior ();

                        if ( behavior == "behavior of disappearance on touch" || behavior == "behavior of disappearance on jump into" ||
                                        std::find( badBoys.begin (), badBoys.end (), behavior ) != badBoys.end () )
                        {
                                freeItem->getBehavior()->changeActivityOfItem( this->switchInRoomIsOn ? Freeze : WakeUp );
                        }
                }
        }

        // Búsqueda en las listas de elementos rejilla de elementos volátiles para congelarlos
        for ( int i = 0; i < room->getTilesX() * room->getTilesY(); i++ )
        {
                for ( std::list< GridItem* >::iterator g = structure[ i ].begin (); g != structure[ i ].end (); ++g )
                {
                        GridItem* gridItem = *g;

                        if ( gridItem != 0 && gridItem->getBehavior() != 0 )
                        {
                                std::string behavior = gridItem->getBehavior()->getNameOfBehavior ();

                                if ( behavior == "behavior of disappearance on touch" || behavior == "behavior of disappearance on jump into" )
                                {
                                        gridItem->getBehavior()->changeActivityOfItem( this->switchInRoomIsOn ? Freeze : Wait );
                                }
                        }
                }
        }
}

bool Mediator::sortGridItemList( GridItem* g1, GridItem* g2 )
{
        return ( g1->getZ() < g2->getZ() + g2->getHeight() );
}

bool Mediator::sortFreeItemList( FreeItem* f1, FreeItem* f2 )
{
        return (
                ( f1->getZ() < f2->getZ() + f2->getHeight() ) ||
                ( f1->getX() < f2->getX() + f2->getWidthX() ) ||
                ( f1->getY() - f1->getWidthY() < f2->getY() )
        );
}

unsigned short Mediator::getBound( const Direction& direction )
{
        return room->bounds[ direction ];
}

void Mediator::setActivePlayer( PlayerItem* playerItem )
{
        this->activePlayer = playerItem;
}

PlayerItem* Mediator::getHiddenPlayer() const
{
        PlayerItem* playerItem = 0;

        for ( size_t i = 0; i < this->playerItems.size(); ++i )
        {
                if ( this->playerItems[ i ] != this->activePlayer )
                        playerItem = this->playerItems[ i ];
        }

        return playerItem;
}

Door* Mediator::getDoor( const Direction& direction )
{
        return room->doors[ direction ];
}

bool EqualItemId::operator() ( Item* item, int id ) const
{
        return ( item->getId() == id );
}

bool EqualPlayerItemId::operator() ( PlayerItem* playerItem, int id ) const
{
        return ( playerItem->getId() == id );
}

bool EqualItemLabel::operator() ( Item* item, const std::string& label ) const
{
        return ( item->getLabel() == label );
}

bool EqualBehaviorOfItem::operator() ( Item* item, const std::string& behavior ) const
{
        return ( item->getBehavior() != 0 && item->getBehavior()->getNameOfBehavior () == behavior );
}

}

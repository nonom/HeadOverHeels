
#include "GameManager.hpp"
#include "Exception.hpp"
#include "Isomot.hpp"
#include "GameFileManager.hpp"
#include "MapManager.hpp"
#include "Room.hpp"
#include "Mediator.hpp"
#include "PlayerItem.hpp"
#include "GuiManager.hpp"
#include "LanguageManager.hpp"
#include "Label.hpp"
#include "InputManager.hpp"
#include "BonusManager.hpp"
#include "SoundManager.hpp"
#include "InputManager.hpp"


namespace isomot
{

GameManager* GameManager::instance = 0;


GameManager::GameManager( )
        : chosenGraphicSet( "gfx" )
        , vidasInfinitas( false )
        , drawBackgroundPicture( true )
        , isomot( new Isomot() )
        , gameFileManager( new GameFileManager( this, isomot ) )
        , headLives( 8 )
        , heelsLives( 8 )
        , highSpeed( 0 )
        , highJumps( 0 )
        , headShield( 0 )
        , heelsShield( 0 )
        , horn( false )
        , handbag( false )
        , donuts( 0 )
        , itemTaken( 0 )
        , visitedRooms( 0 )
        , freePlanets( std::vector< bool >( 5, false ) )
        , takenCrown( false )
        , eatenFish( false )
        , gameOver( false )
        , freedom( false )
        , emperator( false )
        , frameForJail( 0 )
        , frameForBlacktooth( 0 )
        , frameForMarket( 0 )
        , frameForMoon( 0 )
        , frameForByblos( 0 )
        , frameForSafari( 0 )
        , frameForEgyptus( 0 )
        , frameForPenitentiary( 0 )
        , pictureOfHead( 0 )
        , grayPictureOfHead( 0 )
        , pictureOfHeels( 0 )
        , grayPictureOfHeels( 0 )
        , pictureOfBag( 0 )
        , grayPictureOfBag( 0 )
        , pictureOfHorn( 0 )
        , grayPictureOfHorn( 0 )
        , pictureOfDonuts( 0 )
        , grayPictureOfDonuts( 0 )
        , pictureOfGrandesSaltos( 0 )
        , grayPictureOfGrandesSaltos( 0 )
        , pictureOfGranVelocidad( 0 )
        , grayPictureOfGranVelocidad( 0 )
        , pictureOfEscudo( 0 )
        , grayPictureOfEscudo( 0 )
{

}

GameManager::~GameManager( )
{
        delete frameForMoon ;
        delete frameForMarket ;
        delete frameForBlacktooth ;
        delete frameForJail ;
        delete frameForPenitentiary ;
        delete frameForEgyptus ;
        delete frameForSafari ;
        delete frameForByblos ;

        delete pictureOfHead ;
        delete pictureOfHeels ;
        delete grayPictureOfHead ;
        delete grayPictureOfHeels ;

        delete pictureOfBag ;
        delete pictureOfHorn ;
        delete grayPictureOfHorn ;
        delete grayPictureOfBag ;
        delete pictureOfDonuts ;
        delete grayPictureOfDonuts ;

        delete pictureOfGrandesSaltos ;
        delete grayPictureOfGrandesSaltos ;
        delete pictureOfGranVelocidad ;
        delete grayPictureOfGranVelocidad ;
        delete pictureOfEscudo ;
        delete grayPictureOfEscudo ;

        delete isomot ;
}

GameManager* GameManager::getInstance ()
{
        if ( instance == 0 )
        {
                instance = new GameManager();
        }

        return instance;
}

BITMAP * GameManager::refreshPicture ( BITMAP * picture, const char * nameOfPicture )
{
        if ( picture != 0 )
        {
                delete picture;
                picture = 0;
        }

        return load_png( ( gui::GuiManager::getInstance()->getPathToPicturesOfGui() + nameOfPicture ).c_str (), 0 );
}

WhyPause GameManager::begin ()
{
        fprintf ( stdout, "GameManager::begin ()\n" ) ;

        this->vidasInfinitas = false;
        this->headLives = 8;
        this->heelsLives = 8;
        this->highSpeed = 0;
        this->highJumps = 0;
        this->headShield = 0;
        this->heelsShield = 0;
        this->horn = false;
        this->handbag = false;
        this->donuts = 0;
        this->itemTaken = 0;
        this->visitedRooms = 0;
        this->freePlanets.clear();
        this->freePlanets = std::vector< bool >( 5, false );

        refreshImages ();
        refreshBackgroundFrames ();

        Assert< ENullPointer >( this->isomot );
        this->isomot->beginNew ();

        return this->update ();
}

WhyPause GameManager::resume ()
{
        fprintf ( stdout, "GameManager::resume ()\n" ) ;

        refreshImages ();
        refreshBackgroundFrames ();

        Assert< ENullPointer >( this->isomot );
        this->isomot->resume ();

        return this->update ();
}

void GameManager::refreshBackgroundFrames ()
{
        frameForJail = refreshPicture( frameForJail, "jail-frame.png" );
        frameForBlacktooth = refreshPicture( frameForBlacktooth, "blacktooth-frame.png" );
        frameForMarket = refreshPicture( frameForMarket, "market-frame.png" );
        frameForMoon = refreshPicture( frameForMoon, "themoon-frame.png" );
        frameForSafari = refreshPicture( frameForSafari, "safari-frame.png" );
        frameForByblos = refreshPicture( frameForByblos, "byblos-frame.png" );
        frameForPenitentiary = refreshPicture( frameForPenitentiary, "penitentiary-frame.png" );
        frameForEgyptus = refreshPicture( frameForEgyptus, "egyptus-frame.png" );
}

void GameManager::refreshImages ()
{
        pictureOfHead = refreshPicture( pictureOfHead, "head.png" );
        pictureOfHeels = refreshPicture( pictureOfHeels, "heels.png" );
        grayPictureOfHead = refreshPicture( grayPictureOfHead, "grey-head.png" );
        grayPictureOfHeels = refreshPicture( grayPictureOfHeels, "grey-heels.png" );

        pictureOfBag = refreshPicture( pictureOfBag, "handbag.png" );
        pictureOfHorn = refreshPicture( pictureOfHorn, "horn.png" );
        pictureOfDonuts = refreshPicture( pictureOfDonuts, "donuts.png" );
        grayPictureOfHorn = refreshPicture( grayPictureOfHorn, "grey-horn.png" );
        grayPictureOfBag = refreshPicture( grayPictureOfBag, "grey-handbag.png" );
        grayPictureOfDonuts = refreshPicture( grayPictureOfDonuts, "grey-donuts.png" );

        pictureOfGrandesSaltos = refreshPicture( pictureOfGrandesSaltos, "high-jumps.png" );
        grayPictureOfGrandesSaltos = refreshPicture( grayPictureOfGrandesSaltos, "high-jumps.gray.png" );
        if ( grayPictureOfGrandesSaltos == 0 ) grayPictureOfGrandesSaltos = pictureOfGrandesSaltos;

        pictureOfGranVelocidad = refreshPicture( pictureOfGranVelocidad, "high-speed.png" );
        grayPictureOfGranVelocidad = refreshPicture( grayPictureOfGranVelocidad, "high-speed.gray.png" );
        if ( grayPictureOfGranVelocidad == 0 ) grayPictureOfGranVelocidad = pictureOfGranVelocidad;

        pictureOfEscudo = refreshPicture( pictureOfEscudo, "shield.png" );
        grayPictureOfEscudo = refreshPicture( grayPictureOfEscudo, "shield.gray.png" );
        if ( grayPictureOfEscudo == 0 ) grayPictureOfEscudo = pictureOfEscudo;
}

void GameManager::drawAmbianceOfGame ( BITMAP * where )
{
        // scenery of this room
        std::string scenery = this->isomot->getMapManager()->getActiveRoom()->getScenery();

        // empty scenery means it is the final screen
        if ( ! scenery.empty () )
        {
                if ( drawBackgroundPicture )
                {
                        // marco, varía en función del escenario
                        BITMAP * background = 0;

                        if ( scenery == "blacktooth" )
                                background = frameForBlacktooth;
                        else if ( scenery == "jail" )
                                background = frameForJail;
                        else if ( scenery == "market" )
                                background = frameForMarket;
                        else if ( scenery == "themoon" )
                                background = frameForMoon;
                        else if ( scenery == "safari" )
                                background = frameForSafari;
                        else if ( scenery == "byblos" )
                                background = frameForByblos;
                        else if ( scenery == "penitentiary" )
                                background = frameForPenitentiary;
                        else if ( scenery == "egyptus" )
                                background = frameForEgyptus;

                        if ( background != 0 )
                                draw_sprite( where, background, 0, 0 );
                }

                PlayerId playerId = static_cast< PlayerId >( this->isomot->getMapManager()->getActiveRoom()->getMediator()->getActivePlayer()->getLabel() );
                draw_sprite( where, ( ( playerId == Head || playerId == HeadAndHeels ) ? pictureOfHead : grayPictureOfHead ), 161, 425 );
                draw_sprite( where, ( ( playerId == Heels || playerId == HeadAndHeels ) ? pictureOfHeels : grayPictureOfHeels ), 431, 425 );

                draw_sprite( where, ( this->horn ? pictureOfHorn : grayPictureOfHorn ), 33, 425 );

                draw_sprite( where, ( this->handbag ? pictureOfBag : grayPictureOfBag ), 559, 425 );

                std::stringstream ss;

                // vidas de Head
                ss.str( std::string() );
                ss << int( this->headLives );
                gui::Label headLivesLabel( this->headLives > 9 ? 214 : 221, 424, ss.str (), "big", "white", -2 );
                headLivesLabel.draw( where );

                // vidas de Heels
                ss.str( std::string() );
                ss << int( this->heelsLives );
                gui::Label heelsLivesLabel( this->heelsLives > 9 ? 398 : 405, 424, ss.str (), "big", "white", -2 );
                heelsLivesLabel.draw( where );

                // número de rosquillas
                draw_sprite( where, ( this->donuts != 0 ? pictureOfDonuts : grayPictureOfDonuts ), 33, 361 );
                if( this->donuts > 0 )
                {
                        ss.str( std::string() );
                        ss << this->donuts;
                        gui::Label donutsLabel( this->donuts > 9 ? 42 : 49, 372, ss.str (), "regular", "white", -2 );
                        donutsLabel.draw( where );
                }

                // grandes saltos
                draw_sprite( where, ( this->highJumps > 0 ? pictureOfGrandesSaltos : grayPictureOfGrandesSaltos ), 505, 392 );
                if( this->highJumps > 0 )
                {
                        ss.str( std::string() );
                        ss << int( this->highJumps );
                        gui::Label highJumpsLabel( this->highJumps > 9 ? 505 : 512, 393, ss.str (), "regular", "white", -2 );
                        highJumpsLabel.draw( where );
                }

                // gran velocidad
                draw_sprite( where, ( this->highSpeed > 0 ? pictureOfGranVelocidad : grayPictureOfGranVelocidad ), 107, 392 );
                if( this->highSpeed > 0 )
                {
                        ss.str( std::string () );
                        ss << int( this->highSpeed );
                        gui::Label highSpeedLabel( this->highSpeed > 9 ? 107 : 114, 393, ss.str (), "regular", "white", -2 );
                        highSpeedLabel.draw( where );
                }

                // escudo de Head
                draw_sprite( where, ( this->headShield > 0 ? pictureOfEscudo : grayPictureOfEscudo ), 107, 436 );
                if( this->headShield > 0 )
                {
                        int headShieldValue = int( this->headShield * 99.0 / 25.0 );

                        ss.str( std::string() );
                        ss << headShieldValue;
                        gui::Label headShieldLabel( headShieldValue > 9 ? 107 : 114, 437, ss.str (), "regular", "white", -2 );
                        headShieldLabel.draw( where );
                }

                // escudo de Heels
                draw_sprite( where, ( this->headShield > 0 ? pictureOfEscudo : grayPictureOfEscudo ), 505, 436 );
                if( this->heelsShield > 0 )
                {
                        int heelsShieldValue = static_cast< int >( this->heelsShield * 99.0 / 25.0 );

                        ss.str( std::string() );
                        ss << heelsShieldValue;
                        gui::Label heelsShieldLabel( heelsShieldValue > 9 ? 505 : 512, 437, ss.str (), "regular", "white", -2 );
                        heelsShieldLabel.draw( where );
                }

                // elemento que hay dentro del bolso
                if( this->itemTaken != 0 )
                {
                        draw_sprite( where, this->itemTaken, 559, 361 );
                }
        }
        else
        {
                // en la pantalla final del juego se dibuja FREEDOM en la esquina inferior izquierda
                gui::Label freedomLabel( 84, 362, "FREEDOM", "big", "multicolor" );
                freedomLabel.draw( where );
        }

        // muestra todo en pantalla
        acquire_screen();
        blit( where, screen, 0, 0, 0, 0, where->w, where->h );
        release_screen();
}

void GameManager::loadGame ( const std::string& fileName )
{
        isomot->prepare() ;
        gameFileManager->loadGame( fileName );
}

void GameManager::saveGame ( const std::string& fileName )
{
        gameFileManager->saveGame( fileName );
}

void GameManager::addLives ( const PlayerId& player, unsigned char lives )
{
        switch( player )
        {
                case Head:
                        if( this->headLives < 100 )
                        {
                                this->headLives += lives;
                        }
                        break;

                case Heels:
                        if( this->heelsLives < 100 )
                        {
                                this->heelsLives += lives;
                        }
                        break;

                case HeadAndHeels:
                        if( this->headLives < 100 )
                        {
                                this->headLives += lives;
                        }

                        if( this->heelsLives < 100 )
                        {
                                this->heelsLives += lives;
                        }

                        break;

                default:
                        ;
        }
}

void GameManager::loseLife ( const PlayerId& player )
{
        if ( ! vidasInfinitas )
        {
                switch ( player )
                {
                        case Head:
                                if( this->headLives > 0 )
                                {
                                        this->headLives--;
                                }
                                break;

                        case Heels:
                                if( this->heelsLives > 0 )
                                {
                                        this->heelsLives--;
                                }
                                break;

                        case HeadAndHeels:
                                if( this->headLives > 0 )
                                {
                                        this->headLives--;
                                }
                                if( this->heelsLives > 0 )
                                {
                                        this->heelsLives--;
                                }
                                break;

                        default:
                                ;
                }
        }
}

void GameManager::takeTool ( const MagicItem& label )
{
        switch( label )
        {
                case Horn:
                        this->horn = true;
                        break;

                case Handbag:
                        this->handbag = true;
                        break;

                default:
                        ;
        }
}

void GameManager::addHighSpeed ( const PlayerId& player, unsigned int highSpeed )
{
        switch( player )
        {
                case Head:
                case HeadAndHeels:
                        this->highSpeed += highSpeed;
                        if( this->highSpeed > 99 )
                        {
                        this->highSpeed = 99;
                        }
                        break;

                default:
                        ;
        }
}

void GameManager::decreaseHighSpeed ( const PlayerId& player )
{
        switch(player)
        {
                case Head:
                case HeadAndHeels:
                        if( this->highSpeed > 0 )
                        {
                                this->highSpeed--;
                        }
                        break;

                default:
                        ;
        }
}

void GameManager::addHighJumps ( const PlayerId& player, unsigned int highJumps )
{
        switch( player )
        {
                case Heels:
                case HeadAndHeels:
                        this->highJumps += highJumps;
                        if( this->highJumps > 99 )
                        {
                        this->highJumps = 99;
                        }
                        break;

                default:
                        ;
        }
}

void GameManager::decreaseHighJumps ( const PlayerId& player )
{
        switch( player )
        {
                case Heels:
                case HeadAndHeels:
                        if( this->highJumps > 0 )
                        {
                                this->highJumps--;
                        }
                        break;

                default:
                ;
        }
}

void GameManager::addShield ( const PlayerId& player, double shield )
{
        switch( player )
        {
                case Head:
                        this->headShield += shield;
                        if( this->headShield > 25.000 )
                        {
                                this->headShield = 25.000;
                        }
                        break;

                case Heels:
                        this->heelsShield += shield;
                        if( this->heelsShield > 25.000 )
                        {
                                this->heelsShield = 25.000;
                        }
                        break;

                case HeadAndHeels:
                        this->headShield += shield;
                        if( this->headShield > 25.000 )
                        {
                                this->headShield = 25.000;
                        }

                        this->heelsShield += shield;
                        if( this->heelsShield > 25.000 )
                        {
                                this->heelsShield = 25.000;
                        }

                        break;

                default:
                ;
        }
}

void GameManager::decreaseShield ( const PlayerId& player, double shield )
{
        switch( player )
        {
                case Head:
                        this->headShield = shield;
                        if( this->headShield < 0 )
                        {
                                this->headShield = 0;
                        }
                        break;

                case Heels:
                        this->heelsShield = shield;
                        if( this->heelsShield < 0 )
                        {
                                this->heelsShield = 0;
                        }
                        break;

                case HeadAndHeels:
                        this->headShield = shield;
                        if( this->headShield < 0 )
                        {
                                this->headShield = 0;
                        }

                        this->heelsShield = shield;
                        if( this->heelsShield < 0 )
                        {
                                this->heelsShield = 0;
                        }

                        break;

                default:
                ;
        }
}

void GameManager::emptyHandbag ( const PlayerId& player )
{
        switch( player )
        {
                case Heels:
                case HeadAndHeels:
                        this->itemTaken = 0;
                        break;

                default:
                        ;
        }
}

void GameManager::resetFreePlanets ()
{
        for( int i = Blacktooth; i < Safari; ++i )
        {
                this->freePlanets[ i ] = false;
        }
}

void GameManager::liberatePlanet ( const PlanetId& planet, bool now )
{
        if( planet >= Blacktooth && planet <= Safari )
        {
                this->freePlanets[ planet ] = true;
                this->takenCrown = now;
        }
}

bool GameManager::isFreePlanet ( const PlanetId& planet ) const
{
        bool isFree = false;

        if( planet >= Blacktooth && planet <= Safari )
        {
                isFree = this->freePlanets[ planet ];
        }

        return isFree;
}

unsigned short GameManager::freePlanetsCount () const
{
        unsigned short number = 0;

        for( size_t i = 0; i < freePlanets.size(); ++i )
        {
                number += freePlanets[ i ] ? 1 : 0;
        }

        return number;
}

void GameManager::eatFish ( PlayerItem* character, Room* room )
{
        this->eatFish( character, room, character->getX (), character->getY (), character->getZ () ) ;
}

void GameManager::eatFish ( PlayerItem* character, Room* room, int x, int y, int z )
{
        this->eatenFish = true;

        gameFileManager->assignFishData(
                room->getIdentifier (), character->getLabel (),
                        x, y, z, character->getDirection () ) ;
}

WhyPause GameManager::update ()
{
        WhyPause why = Nevermind;

        // mientras el usuario no pulse la tecla de abandono y lo confirme con Escape,
        // el juego se actualiza periódicamente
        while ( why == Nevermind )
        {
                if ( ! key[ InputManager::getInstance()->getUserKey( "halt" ) ] && ! this->takenCrown && ! this->eatenFish && ! this->gameOver )
                {
                        // actualiza la vista isométrica
                        BITMAP* view = this->isomot->update();

                        // se ha podido actualizar
                        if ( view != 0 )
                        {
                                this->drawAmbianceOfGame( view );
                                sleep( 10 );
                        }
                        else
                        {
                                // la partida ha terminado
                                this->gameOver = true;
                        }
                }
                else
                {
                        // detiene el juego
                        why = this->pause ();
                }
        }

        return why;
}

WhyPause GameManager::pause ()
{
        bool exit = false;
        bool confirm = false;
        WhyPause why = Nevermind;

        // detiene el motor isométrico
        this->isomot->pause();

        // da tiempo a que el usuario vea la parada
        clear_keybuf();

        // el usuario acaba de liberar un planeta, se muestra la pantalla de los planetas
        if ( this->takenCrown )
        {
                why = FreePlanet;
                this->takenCrown = false;
        }
        // el usuario acaba de comer un pez, se le da la opción de grabar la partida
        else if ( this->eatenFish )
        {
                this->eatenFish = false;
                isomot::InputManager* inputManager = InputManager::getInstance();

                // presenta las intrucciones en pantalla
                gui::LanguageText* text = 0;
                gui::LanguageManager* language = gui::GuiManager::getInstance()->getLanguageManager();

                text = language->findLanguageString( "save-game" );
                int deltaY = text->getY();
                for ( size_t i = 0; i < text->getLinesCount(); i++ )
                {
                        gui::LanguageLine* line = text->getLine( i );
                        gui::Label label( 0, 0, line->text, line->font, line->color );
                        label.changePosition( ( 640 - label.getWidth() ) >> 1, deltaY );
                        deltaY += label.getHeight() * 3 / 4;
                        acquire_screen();
                        label.draw( screen );
                        release_screen();
                }

                text = language->findLanguageString( "confirm-resume" );
                deltaY = text->getY();
                for ( size_t i = 0; i < text->getLinesCount(); i++ )
                {
                        gui::LanguageLine* line = text->getLine( i );
                        gui::Label label( 0, 0, line->text, line->font, line->color );
                        label.changePosition( ( 640 - label.getWidth() ) >> 1, deltaY );
                        deltaY += label.getHeight() * 3 / 4;
                        acquire_screen();
                        label.draw( screen );
                        release_screen();
                }

                clear_keybuf();

                // mientras el usuario no elija una de las dos opciones no se hará nada
                while ( ! confirm && ! exit )
                {
                        if ( keypressed() )
                        {
                                int key = readkey() >> 8;

                                // si se pulsa SPACE se mostrará la interfaz para la grabación de la partida
                                if ( key == KEY_SPACE )
                                {
                                        exit = true;
                                        why = SaveGame;
                                }
                                else if ( key != inputManager->getUserKey( "movenorth" ) &&
                                          key != inputManager->getUserKey( "movesouth" ) &&
                                          key != inputManager->getUserKey( "moveeast" ) &&
                                          key != inputManager->getUserKey( "movewest" ) &&
                                          key != inputManager->getUserKey( "jump" ) &&
                                          key != inputManager->getUserKey( "take" ) &&
                                          key != inputManager->getUserKey( "take-jump" ) &&
                                          key != inputManager->getUserKey( "swap" ) &&
                                          key != inputManager->getUserKey( "shoot" ) &&
                                          key != KEY_ESC )
                                {
                                        confirm = true;
                                        this->isomot->resume();
                                }
                        }

                        // sondea la decisión del usuario cada 10 milisegundos
                        // poll the decision of user every 10 milliseconds
                        sleep( 10 );
                }
        }
        // el usuario ha acabado con las vidas de todos los jugadores
        else if ( this->gameOver )
        {
                this->gameOver = false;
                why = GameOver;
                this->visitedRooms = this->isomot->getMapManager()->countVisitedRooms();
                this->isomot->reset();
        }
        // el usuario ha llegado a Freedom pero le faltaron coronas
        else if ( this->freedom )
        {
                this->freedom = false;
                why = Freedom;
                this->visitedRooms = this->isomot->getMapManager()->countVisitedRooms();
                this->isomot->reset();
        }
        // el usuario ha acabado el juego
        else if ( this->emperator )
        {
                this->emperator = false;
                why = Sucess;
                this->visitedRooms = this->isomot->getMapManager()->countVisitedRooms();
                this->isomot->reset();
        }
        // el usuario ha pulsado la tecla de pausa
        else
        {
                // Detiene todos los sonidos
                SoundManager::getInstance()->stopEverySound ();

                // Presenta las intrucciones en pantalla
                gui::LanguageText* text = 0;
                gui::LanguageManager* language = gui::GuiManager::getInstance()->getLanguageManager();

                text = language->findLanguageString( "confirm-quit" );
                int deltaY = text->getY();
                for ( size_t i = 0; i < text->getLinesCount(); i++ )
                {
                        gui::LanguageLine* line = text->getLine( i );
                        gui::Label label( 0, 0, line->text, line->font, line->color );
                        label.changePosition( ( 640 - label.getWidth() ) >> 1, deltaY );
                        deltaY += label.getHeight() * 3 / 4;
                        acquire_screen();
                        label.draw( screen );
                        release_screen();
                }

                text = language->findLanguageString( "confirm-resume" );
                deltaY = text->getY();
                for ( size_t i = 0; i < text->getLinesCount(); i++ )
                {
                        gui::LanguageLine* line = text->getLine( i );
                        gui::Label label( 0, 0, line->text, line->font, line->color );
                        label.changePosition( ( 640 - label.getWidth() ) >> 1, deltaY );
                        deltaY += label.getHeight() * 3 / 4;
                        acquire_screen();
                        label.draw( screen );
                        release_screen();
                }

                // mientras el usuario no elija una de las dos opciones no se hará nada
                while ( ! confirm && ! exit )
                {
                        if ( keypressed() )
                        {
                                // Si ha pulsado Escape se suspende la partida en curso
                                if ( readkey() >> 8 == KEY_ESC )
                                {
                                        exit = true;
                                        why = GameOver;
                                        this->visitedRooms = this->isomot->getMapManager()->countVisitedRooms();
                                        this->isomot->reset();
                                }
                                // Si se pulsa cualquier otra tecla, prosigue la partida en curso
                                else
                                {
                                        confirm = true;
                                        this->isomot->resume();
                                }
                        }

                        // sondea la decisión del usuario cada 10 milisegundos
                        sleep( 10 );
                }
        }

        return why;
}

unsigned char GameManager::getLives ( const PlayerId& player ) const
{
        unsigned char lives = 0;

        switch ( player )
        {
                case Head:
                        lives = this->headLives;
                        break;

                case Heels:
                        lives = this->heelsLives;
                        break;

                case HeadAndHeels:
                        lives = std::min( this->headLives, this->heelsLives );
                        break;

                default:
                        ;
        }

        return lives;
}

double GameManager::getShield ( const PlayerId& player ) const
{
        double time = 0.0;

        switch ( player )
        {
                case Head:
                        time = this->headShield;
                        break;

                case Heels:
                        time = this->heelsShield;
                        break;

                case HeadAndHeels:
                        time = std::max( this->headShield, this->heelsShield );
                        break;

                default:
                        ;
        }

        return time;
}

std::vector< short > GameManager::hasTool ( const PlayerId& player ) const
{
        std::vector< short > tools;

        switch ( player )
        {
                case Head:
                        if ( this->horn )
                        {
                                tools.push_back( Horn );
                        }
                        break;

                case Heels:
                        if ( this->handbag )
                        {
                                tools.push_back( Handbag );
                        }
                        break;

                case HeadAndHeels:
                        if ( this->horn )
                        {
                                tools.push_back( Horn );
                        }
                        if ( this->handbag )
                        {
                                tools.push_back( Handbag );
                        }
                        break;

                default:
                        ;
        }

        return tools;
}

unsigned short GameManager::getDonuts ( const PlayerId& player ) const
{
        return ( player == Head || player == HeadAndHeels ? this->donuts : 0 );
}

size_t GameManager::countFreePlanets () const
{
        return std::count( this->freePlanets.begin (), this->freePlanets.end (), true );
}

}
